"""Fast builder checks; no network or application compilation."""
import importlib.util
import io
import hashlib
from pathlib import Path, PureWindowsPath
import tarfile
import subprocess
import shutil
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('audio_build', Path(__file__).with_name('build.py'))
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)


def archive(entries):
    output = io.BytesIO()
    with tarfile.open(fileobj=output, mode='w') as f:
        for name, text, link in entries:
            info = tarfile.TarInfo(name)
            if link:
                info.type = tarfile.SYMTYPE
                info.linkname = text
                f.addfile(info)
            else:
                info.size = len(text)
                f.addfile(info, io.BytesIO(text))
    return output.getvalue()


class BuilderTests(unittest.TestCase):
    def test_channel_index_fix_is_exact_idempotent_and_rejects_drift(self):
        original = b'#include <cmath>\n' + b'\n'.join(
            ('static_cast<int>(std::floor(mpcMono' + direction + 'ChannelIndices[i] / 2.f))').encode()
            for direction in ('Input', 'Output'))
        expected = b'mpcMonoInputChannelIndices[i] / 2\nmpcMonoOutputChannelIndices[i] / 2'
        fix = {'path': 'server.cpp',
               'before_sha256': hashlib.sha256(original).hexdigest(),
               'after_sha256': hashlib.sha256(expected).hexdigest()}
        for newline in [b'\n', b'\r\n']:
            with self.subTest(newline=newline), tempfile.TemporaryDirectory() as d:
                root = Path(d)
                source = root / fix['path']
                source.write_bytes(original.replace(b'\n', newline))
                build.apply_channel_index_fix(root, fix)
                self.assertEqual(source.read_bytes(), expected)
                build.apply_channel_index_fix(root, fix)
                self.assertEqual(source.read_bytes(), expected)
                source.write_bytes(expected.replace(b'\n', newline))
                build.apply_channel_index_fix(root, fix)
                self.assertEqual(source.read_bytes(), expected)
                altered = original.replace(b'\n', newline) + b'// changed'
                source.write_bytes(altered)
                with self.assertRaisesRegex(RuntimeError, 'LF-normalized SHA-256'):
                    build.apply_channel_index_fix(root, fix)
                self.assertEqual(source.read_bytes(), altered)

    @unittest.skipUnless(shutil.which('cmake'), 'CMake is required for compiler flag checks')
    def test_msvc_defaults_survive_sse2_and_stale_cache_overrides(self):
        # Run CMake's real MSVC flag initialization without a Windows compiler.
        with tempfile.TemporaryDirectory() as d:
            script = Path(d) / 'flags.cmake'
            script.write_text('''cmake_minimum_required(VERSION 3.24)
set(CMAKE_C_COMPILER_ID MSVC)
set(CMAKE_CXX_COMPILER_ID MSVC)
set(CMAKE_C_COMPILER_VERSION 19.42)
set(CMAKE_CXX_COMPILER_VERSION 19.42)
set(CMAKE_SYSTEM_NAME Windows)
include(Platform/Windows-MSVC)
# This test only checks flags; it does not compile resources.
macro(__windows_compiler_msvc_enable_rc flags)
endmacro()
include(Platform/Windows-MSVC-C)
include(Platform/Windows-MSVC-CXX)
include(CMakeInitializeConfigs)
cmake_initialize_per_config_variable(CMAKE_C_FLAGS "C flags")
cmake_initialize_per_config_variable(CMAKE_CXX_FLAGS "C++ flags")
foreach(flag /arch:SSE2 /EHsc /D_WINDOWS)
    if(NOT " ${CMAKE_CXX_FLAGS} " MATCHES " ${flag} ")
        message(FATAL_ERROR "Missing ${flag}: ${CMAKE_CXX_FLAGS}")
    endif()
endforeach()
if(NOT CMAKE_C_FLAGS MATCHES "/arch:SSE2" OR CMAKE_C_FLAGS MATCHES "/EHsc")
    message(FATAL_ERROR "Unexpected C flags: ${CMAKE_C_FLAGS}")
endif()
''')
            for stale in [[], ['-DCMAKE_C_FLAGS=/arch:SSE2', '-DCMAKE_CXX_FLAGS=/arch:SSE2']]:
                result = subprocess.run(['cmake', *stale, *build.windows_compiler_flags(), '-P', str(script)],
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_windows_short_path_skips_occupied_drives_and_cleans_up_on_failure(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            occupied = 1 << (ord('Z') - ord('A'))
            with patch.object(build, 'windows_drive_mask', return_value=occupied), \
                    patch.object(build.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0)) as run:
                with self.assertRaisesRegex(RuntimeError, 'build failed'):
                    with build.short_windows_root(root) as mapped:
                        self.assertEqual(mapped.as_posix(), 'Y:/' if build.os.name == 'nt' else 'Y:')
                        raise RuntimeError('build failed')
                self.assertEqual(run.call_args_list[0].args[0], ['subst', 'Y:', str(root)])
                self.assertEqual(run.call_args_list[1].args[0], ['subst', 'Y:', '/D'])

    def test_windows_short_path_retries_failed_mapping_and_releases_only_its_own(self):
        with tempfile.TemporaryDirectory() as d:
            failed = subprocess.CompletedProcess([], 1, '', 'drive unavailable')
            succeeded = subprocess.CompletedProcess([], 0, '', '')
            with patch.object(build, 'windows_drive_mask', return_value=0), \
                    patch.object(build.subprocess, 'run', side_effect=[failed, succeeded, succeeded]) as run:
                with build.short_windows_root(Path(d)):
                    pass
                self.assertEqual([call.args[0][1] for call in run.call_args_list], ['Z:', 'Y:', 'Y:'])
                self.assertEqual(run.call_args_list[-1].args[0], ['subst', 'Y:', '/D'])

    def test_windows_short_path_does_not_touch_occupied_drives(self):
        with patch.object(build, 'windows_drive_mask', return_value=(1 << 26) - 1), \
                patch.object(build.subprocess, 'run') as run:
            with self.assertRaisesRegex(RuntimeError, 'No free drive letter'):
                with build.short_windows_root(Path('unused')):
                    self.fail('Unexpected mapping')
            run.assert_not_called()

    def test_relocated_build_discards_only_stale_build_directory(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            source = root / 'source'
            source.mkdir()
            directory = root / 'build'
            directory.mkdir()
            cache = directory / 'CMakeCache.txt'
            cache.write_text('CMAKE_CACHEFILE_DIR:INTERNAL=' + directory.as_posix() + '\n')
            build.discard_relocated_build(directory)
            self.assertTrue(cache.exists())
            cache.write_text('CMAKE_CACHEFILE_DIR:INTERNAL=C:/old/long/path\n')
            build.discard_relocated_build(directory)
            self.assertFalse(directory.exists())
            self.assertTrue(source.exists())

    def test_changed_dependency_discards_nested_cmake_caches(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            directory = root / 'build'
            directory.mkdir()
            cache = directory / 'CMakeCache.txt'
            cache.write_text('FETCHCONTENT_SOURCE_DIR_MPC:PATH=/old/mpc\n')
            build.discard_changed_dependency_build(directory, ['-DFETCHCONTENT_SOURCE_DIR_MPC=/old/mpc'])
            self.assertTrue(cache.exists())
            build.discard_changed_dependency_build(directory, ['-DFETCHCONTENT_SOURCE_DIR_MPC=/new/mpc'])
            self.assertFalse(directory.exists())

    def test_retry_detects_juceaide_cache_after_failed_configure(self):
        with tempfile.TemporaryDirectory() as d:
            directory = Path(d) / 'build'
            nested = directory / '_deps/juce-build/tools'
            nested.mkdir(parents=True)
            (directory / 'CMakeCache.txt').write_text('FETCHCONTENT_SOURCE_DIR_JUCE:PATH=/new/JUCE\n')
            (nested / 'CMakeCache.txt').write_text('CMAKE_HOME_DIRECTORY:INTERNAL=/old/JUCE\n')
            build.discard_changed_dependency_build(directory, ['-DFETCHCONTENT_SOURCE_DIR_JUCE=/new/JUCE'])
            self.assertFalse(directory.exists())

    def test_hash_uses_same_order_for_windows_and_posix_paths(self):
        # Exercise Windows' native comparison rules even on a macOS builder.
        class WindowsOrderedPath(type(Path())):
            def __lt__(self, other):
                return PureWindowsPath(self) < PureWindowsPath(other)

        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            for name in ['Z.hpp', 'a.hpp', 'a/file.hpp', 'a-file.hpp']:
                file = root / name
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_bytes(name.encode())
            self.assertEqual(build.tree_hash(root), build.tree_hash(WindowsOrderedPath(d)))

    def test_archive_rejects_traversal_and_external_links(self):
        for entry in [('../escape', b'x', False), ('link', '../escape', True)]:
            with tempfile.TemporaryDirectory() as d:
                destination = Path(d) / 'source'
                with self.assertRaises(RuntimeError):
                    build.extract(archive([entry]), destination)
                self.assertFalse(destination.exists())

    def test_windows_materializes_internal_symlink(self):
        with tempfile.TemporaryDirectory() as d:
            destination = Path(d) / 'source'
            with patch.object(build.os, 'name', 'nt'):
                build.extract(archive([('original', b'contents', False), ('nested/link', '../original', True)]), destination)
            self.assertEqual((destination / 'nested/link').read_bytes(), b'contents')
            self.assertFalse((destination / 'nested/link').is_symlink())

    def audit_fixture(self, directory, machine=0x14c):
        root = Path(directory)
        package = root / 'package'
        package.mkdir()
        tools = root / 'tools'
        tools.mkdir()
        (tools / 'dumpbin.exe').touch()
        cache = root / 'build'
        cache.mkdir()
        (cache / 'CMakeCache.txt').write_text('CMAKE_LINKER:FILEPATH=' + str(tools / 'link.exe') + '\n')
        pe = bytearray(128)
        pe[0x3c:0x40] = (64).to_bytes(4, 'little')
        pe[64:68] = b'PE\0\0'
        pe[68:70] = machine.to_bytes(2, 'little')
        (package / 'before.exe').write_bytes(pe)
        return package, cache

    def test_windows_audit_accepts_system_graphics_libraries(self):
        with tempfile.TemporaryDirectory() as d:
            package, cache = self.audit_fixture(d)
            manifest = {}
            with patch.object(build.subprocess, 'check_output', return_value='    KERNEL32.dll\n    DWrite.dll\n    dxgi.dll\n'):
                build.audit_windows(package, cache, manifest)
            self.assertIn('DWrite.dll', manifest['windows_imports']['before.exe'])
            self.assertTrue((package / 'before-imports.txt').exists())

    def test_windows_audit_rejects_wrong_architecture_and_external_runtime(self):
        with tempfile.TemporaryDirectory() as d:
            package, cache = self.audit_fixture(d, machine=0x8664)
            with self.assertRaisesRegex(RuntimeError, 'Not an x86'):
                build.audit_windows(package, cache, {})
        with tempfile.TemporaryDirectory() as d:
            package, cache = self.audit_fixture(d)
            with patch.object(build.subprocess, 'check_output', return_value='    VCRUNTIME140.dll\n'):
                with self.assertRaisesRegex(RuntimeError, 'runtime dependencies'):
                    build.audit_windows(package, cache, {})

    def test_hash_detects_source_change_but_ignores_python_cache(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / 'source.cpp').write_bytes(b'one')
            before = build.tree_hash(root)
            (root / '__pycache__').mkdir()
            (root / '__pycache__/file.pyc').write_bytes(b'cache')
            self.assertEqual(before, build.tree_hash(root))
            (root / 'source.cpp').write_bytes(b'two')
            self.assertNotEqual(before, build.tree_hash(root))


if __name__ == '__main__':
    unittest.main()
