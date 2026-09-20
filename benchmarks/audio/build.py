#!/usr/bin/env python3
"""Build exact A/B exports. Python is required on the builder, never the test PC."""
import argparse
from contextlib import contextmanager, nullcontext
import hashlib
import io
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import tarfile
import urllib.request

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
REVISIONS = {'before': 'eeb8d07c7d5e2166fc2d2a1dd2d3febf491dd010',
             'after': 'a45d440806f43b9702947ca22cc648aa23cfa5eb'}


def run(*args, **kwargs):
    print('+', ' '.join(map(str, args)), flush=True)
    return subprocess.run(list(map(str, args)), check=True, **kwargs)


def windows_drive_mask():
    import ctypes
    return ctypes.windll.kernel32.GetLogicalDrives()


@contextmanager
def short_windows_root(root):
    # MSBuild's file tracker still hits MAX_PATH inside nested JUCE builds.
    # SUBST keeps the files in the requested output directory without admin
    # privileges. Never resolve the yielded path back to its physical location.
    occupied = windows_drive_mask()
    failures = []
    for letter in reversed('DEFGHIJKLMNOPQRSTUVWXYZ'):
        if occupied & (1 << (ord(letter) - ord('A'))):
            continue
        drive = letter + ':'
        result = subprocess.run(['subst', drive, str(root)], capture_output=True, text=True)
        if result.returncode:
            failures.append(result.stderr.strip() or result.stdout.strip())
            continue
        print('Windows build path:', drive + '/', '->', root, flush=True)
        try:
            yield Path(drive + '/')
        finally:
            run('subst', drive, '/D')
        return
    raise RuntimeError('No free drive letter could be mapped for the Windows build: ' + '; '.join(failures))


def discard_relocated_build(build):
    cache = build / 'CMakeCache.txt'
    if not cache.exists():
        return
    for line in cache.read_text().splitlines():
        if line.startswith('CMAKE_CACHEFILE_DIR:INTERNAL='):
            previous = line.split('=', 1)[1].replace('\\', '/').rstrip('/').casefold()
            if previous != build.as_posix().rstrip('/').casefold():
                print('Discarding CMake build with a different cached path:', build, flush=True)
                shutil.rmtree(build)
            break


def discard_changed_dependency_build(build, overrides):
    cache = build / 'CMakeCache.txt'
    if not cache.exists():
        return
    cached = {}
    for line in cache.read_text().splitlines():
        if line.startswith('FETCHCONTENT_SOURCE_DIR_') and '=' in line:
            key, value = line.split('=', 1)
            cached[key.split(':', 1)[0]] = value.replace('\\', '/')
    # A failed configure may update the parent cache before juceaide rejects
    # its old source path. Inspect that nested cache as well on a retry.
    juce_cache = build / '_deps/juce-build/tools/CMakeCache.txt'
    if juce_cache.exists():
        for line in juce_cache.read_text().splitlines():
            if line.startswith('CMAKE_HOME_DIRECTORY:INTERNAL='):
                cached['FETCHCONTENT_SOURCE_DIR_JUCE'] = line.split('=', 1)[1].replace('\\', '/')
    for override in overrides:
        key, value = override.removeprefix('-D').split('=', 1)
        if key in cached and cached[key] != value.replace('\\', '/'):
            print('Discarding CMake build with changed dependency sources:', build, flush=True)
            shutil.rmtree(build)
            return


def windows_compiler_flags():
    # FLAGS replaces MSVC defaults (including /EHsc). INIT adds SSE2 before
    # CMake appends those defaults. Reset old cached overrides on retries.
    return ['-U', 'CMAKE_C_FLAGS', '-U', 'CMAKE_CXX_FLAGS',
            '-DCMAKE_C_FLAGS_INIT=/arch:SSE2', '-DCMAKE_CXX_FLAGS_INIT=/arch:SSE2']


def tree_hash(root):
    h = hashlib.sha256()
    # Path ordering is case-insensitive on Windows. Hash in the same
    # case-sensitive component order on every builder, matching the lock file.
    for p in sorted(root.rglob('*'), key=lambda p: p.relative_to(root).parts):
        if p.is_file() and p.name != '.DS_Store' and '__pycache__' not in p.parts and 'results' not in p.relative_to(root).parts:
            h.update(p.relative_to(root).as_posix().encode() + b'\0' + p.read_bytes())
    return h.hexdigest()


def apply_channel_index_fix(root, fix):
    source = root / fix['path']
    original = source.read_bytes()
    # git archive honors core.autocrlf, including in a bare Windows cache.
    # Canonicalize only line endings; all other source bytes remain locked.
    data = original.replace(b'\r\n', b'\n')
    digest = hashlib.sha256(data).hexdigest()
    if digest == fix['after_sha256']:
        if data != original:
            source.write_bytes(data)
        return
    if digest != fix['before_sha256']:
        raise RuntimeError('Unexpected MPC source for channel-index fix: '
                           f'{source}; LF-normalized SHA-256 {digest}; '
                           f"expected {fix['before_sha256']} or {fix['after_sha256']}")
    data = data.replace(b'#include <cmath>\n', b'')
    for side in ['Input', 'Output']:
        old = ('static_cast<int>(std::floor(mpcMono' + side + 'ChannelIndices[i] / 2.f))').encode()
        if data.count(old) != 1:
            raise RuntimeError('Channel-index fix does not match source')
        data = data.replace(old, ('mpcMono' + side + 'ChannelIndices[i] / 2').encode())
    if hashlib.sha256(data).hexdigest() != fix['after_sha256']:
        raise RuntimeError('Channel-index fix result differs from lock')
    source.write_bytes(data)


def extract(data, dest):
    temporary = dest.with_name(dest.name + '.extracting')
    if temporary.exists():
        shutil.rmtree(temporary)
    temporary.mkdir(parents=True, exist_ok=True)
    with tarfile.open(fileobj=io.BytesIO(data)) as archive:
        for member in archive.getmembers():
            target = (temporary / member.name).resolve()
            if not target.is_relative_to(temporary.resolve()):
                raise RuntimeError('Unsafe archive member: ' + member.name)
            if member.issym() or member.islnk():
                base = target.parent if member.issym() else temporary
                if not (base / member.linkname).resolve().is_relative_to(temporary.resolve()):
                    raise RuntimeError('Unsafe archive link: ' + member.name)
        members = archive.getmembers()
        links = [m for m in members if m.issym() or m.islnk()] if os.name == 'nt' else []
        archive.extractall(temporary, members=[m for m in members if m not in links],
                           **({'filter': 'data'} if hasattr(tarfile, 'data_filter') else {}))
        # Windows CI need not grant symlink privileges: materialize safe links.
        for member in links:
            destination = temporary / member.name
            base = destination.parent if member.issym() else temporary
            source = (base / member.linkname).resolve()
            destination.parent.mkdir(parents=True, exist_ok=True)
            if source.is_dir():
                shutil.copytree(source, destination)
            else:
                shutil.copy2(source, destination)
    temporary.rename(dest)


def git_export(local, url, revision, dest, cache):
    if dest.exists():
        return
    repo = local
    present = repo.exists() and subprocess.run(
        ['git', '-C', str(repo), 'cat-file', '-e', revision + '^{commit}'],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
    if not present:
        repo = cache / hashlib.sha256(url.encode()).hexdigest()[:16]
        if not repo.exists():
            run('git', 'init', '--bare', repo)
        run('git', '-C', repo, 'fetch', '--depth=1', url, revision)
    data = subprocess.check_output(['git', '-C', str(repo), 'archive', revision])
    extract(data, dest)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, default=REPO / 'build/audio-benchmark')
    p.add_argument('--generator', default='Visual Studio 17 2022' if os.name == 'nt' else 'Ninja')
    p.add_argument('--jobs', type=int, default=2)
    p.add_argument('--smoke', action='store_true')
    args = p.parse_args()
    root = args.output.resolve()
    root.mkdir(parents=True, exist_ok=True)
    with short_windows_root(root) if os.name == 'nt' else nullcontext(root) as work_root:
        build_comparison(args, work_root)


def build_comparison(args, root):
    if shutil.which('ccache'):
        os.environ['CCACHE_DIR'] = str(root / 'compiler-cache')
        os.environ['CCACHE_BASEDIR'] = str(root)
        os.environ['CCACHE_NOHASHDIR'] = 'true'
    dependencies = json.loads((HERE / 'dependencies.json').read_text())
    lock_id = hashlib.sha256((HERE / 'dependencies.json').read_bytes()).hexdigest()
    # Keep git/archive source paths stable when adding separately pinned raw files.
    archive_lock = (json.dumps([d for d in dependencies if 'files' not in d], indent=2) + '\n').encode()
    sources = root / ('sources-' + hashlib.sha256(archive_lock).hexdigest()[:12])
    overrides = []
    for dep in dependencies:
        dest = sources / dep['path']
        if 'files' in dep:
            dest.mkdir(parents=True, exist_ok=True)
            for file in dep['files']:
                target = dest / file['name']
                if not target.exists():
                    local = REPO / dep['path'] / file['name']
                    if local.exists() and hashlib.sha256(local.read_bytes()).hexdigest() == file['sha256']:
                        shutil.copy2(local, target)
                    else:
                        with urllib.request.urlopen(file['url']) as response:
                            target.write_bytes(response.read())
                if hashlib.sha256(target.read_bytes()).hexdigest() != file['sha256']:
                    raise RuntimeError('Raw dependency hash mismatch: ' + str(target))
            continue
        if 'revision' in dep:
            git_export(REPO / dep['path'], dep['url'], dep['revision'], dest, root / 'git-cache')
            if 'channel_index_fix' in dep:
                apply_channel_index_fix(dest, dep['channel_index_fix'])
        else:
            if not dest.exists():
                local = REPO / dep['path']
                if local.exists() and tree_hash(local) == dep['tree_sha256']:
                    shutil.copytree(local, dest)
                else:
                    with urllib.request.urlopen(dep['url']) as response:
                        data = response.read()
                    tmp = sources / 'json-download'
                    if tmp.exists():
                        shutil.rmtree(tmp)
                    extract(data, tmp)
                    candidates = [tmp] + [x for x in tmp.iterdir() if x.is_dir()]
                    match = next((x for x in candidates if tree_hash(x) == dep['tree_sha256']), None)
                    if match is None:
                        raise RuntimeError('Downloaded JSON source differs from lock file')
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    shutil.move(str(match), dest)
            if tree_hash(dest) != dep['tree_sha256']:
                raise RuntimeError('JSON content hash mismatch')
        overrides.append('-DFETCHCONTENT_SOURCE_DIR_' + dep['name'].upper() + '=' + dest.as_posix())
    package = root / 'package'
    package.mkdir(exist_ok=True)
    manifest = {'schema': 1, 'revisions': REVISIONS, 'dependencies': dependencies,
                'dependency_lock_sha256': lock_id, 'builds': {},
                'harness_sha256': tree_hash(HERE), 'builder': platform.platform(),
                'asio_enabled': False,
                'allocation_coverage': 'C++ new/new[] (including aligned) on callback thread; excludes malloc and OS allocators'}
    for label, revision in REVISIONS.items():
        stage = root / label / 'source'
        git_export(REPO, 'https://github.com/izzyreal/vmpc-juce.git', revision, stage, root / 'git-cache')
        for dep in dependencies:
            if 'files' in dep:
                shutil.copytree(sources / dep['path'], stage / dep['path'], dirs_exist_ok=True)
        # Restore the historical CMake file before adding the identical overlay.
        pristine_cmake = stage / '.benchmark-original-CMakeLists.txt'
        if not pristine_cmake.exists():
            # Old local exports may already contain the previous overlay.
            original = (stage / 'CMakeLists.txt').read_text().split('\ninclude(benchmarks/audio/Targets.cmake)')[0]
            pristine_cmake.write_text(original)
        cmake = pristine_cmake.read_text()
        # Device drivers are outside this processor-only benchmark. Avoid an unpinned SDK download.
        cmake = cmake.replace('JUCE_ASIO=1', 'JUCE_ASIO=0').replace('    getAndIncludeASIOSDK(asiosdk_2.3.3_2019-06-14)', '    # ASIO disabled in the benchmark overlay')
        (stage / 'CMakeLists.txt').write_text(cmake + '\ninclude(benchmarks/audio/Targets.cmake)\n')
        overlay = stage / 'benchmarks/audio'
        if overlay.exists():
            shutil.rmtree(overlay)
        shutil.copytree(HERE, overlay, ignore=shutil.ignore_patterns('__pycache__', 'results'))
        build = root / label / 'build'
        if os.name == 'nt':
            discard_relocated_build(build)
        discard_changed_dependency_build(build, overrides)
        options = ['-DCMAKE_BUILD_TYPE=Release', '-DVMPC_BUILD_AUDIO_BENCHMARK=ON',
                   '-DVMPC_BUILD_TESTS=OFF', '-DJUCE_COPY_PLUGIN_AFTER_BUILD=OFF',
                   '-DFETCHCONTENT_FULLY_DISCONNECTED=ON', '-DFETCHCONTENT_CACHE_ROOT=',
                   '-DVMPC_BENCHMARK_REVISION=' + revision] + overrides
        if shutil.which('ccache'):
            options += ['-DCMAKE_CXX_COMPILER_LAUNCHER=ccache', '-DCMAKE_C_COMPILER_LAUNCHER=ccache']
        if os.name == 'nt':
            options += ['-A', 'Win32', '-DVMPC2000XL_WIN7=1',
                        '-DCMAKE_DISABLE_FIND_PACKAGE_ZLIB=ON', '-DCMAKE_DISABLE_FIND_PACKAGE_Iconv=ON'] + windows_compiler_flags()
        elif platform.system() == 'Darwin':
            options += ['-DCMAKE_OSX_ARCHITECTURES=' + platform.machine()]
        run('cmake', '-S', stage, '-B', build, '-G', args.generator, *options)
        run('cmake', '--build', build, '--config', 'Release', '--parallel', args.jobs,
            '--target', 'vmpc-audio-worker', 'vmpc-audio-benchmark', 'vmpc-audio-routing-test')
        binary_dir = build / 'Release' if (build / 'Release').is_dir() else build
        run(binary_dir / ('vmpc-audio-routing-test.exe' if os.name == 'nt' else 'vmpc-audio-routing-test'))
        info = json.loads((build / 'benchmark-build-Release.json').read_text())
        keys = {'CMAKE_C_FLAGS', 'CMAKE_CXX_FLAGS', 'CMAKE_C_FLAGS_RELEASE', 'CMAKE_CXX_FLAGS_RELEASE',
                'CMAKE_EXE_LINKER_FLAGS', 'CMAKE_EXE_LINKER_FLAGS_RELEASE', 'CMAKE_MSVC_RUNTIME_LIBRARY',
                'CMAKE_OSX_DEPLOYMENT_TARGET', 'CMAKE_GENERATOR_PLATFORM', 'CMAKE_VS_PLATFORM_TOOLSET'}
        info['effective_flags'] = {}
        for line in (build / 'CMakeCache.txt').read_text().splitlines():
            if '=' in line and ':' in line.split('=', 1)[0]:
                key = line.split(':', 1)[0]
                if key in keys:
                    info['effective_flags'][key] = line.split('=', 1)[1]
        if os.name == 'nt' and info['pointer_bytes'] != 4:
            raise RuntimeError('Windows benchmark must be 32-bit')
        info['cmake_options'] = [x for x in options if not x.startswith('-DFETCHCONTENT_SOURCE_DIR_') and not x.startswith('-DVMPC_BENCHMARK_REVISION=')]
        if tree_hash(overlay) != manifest['harness_sha256']:
            raise RuntimeError('Harness changed during the build; rerun with a stable checkout')
        manifest['builds'][label] = info
        if platform.system() == 'Darwin':
            destination = package / (label + '.app')
            if destination.exists():
                shutil.rmtree(destination)
            shutil.copytree(binary_dir / 'vmpc-audio-worker.app', destination)
            executable = destination / 'Contents/MacOS/vmpc-audio-worker'
        else:
            suffix = '.exe' if os.name == 'nt' else ''
            executable = package / (label + suffix)
            shutil.copy2(binary_dir / ('vmpc-audio-worker' + suffix), executable)
        info['binary_sha256'] = hashlib.sha256(executable.read_bytes()).hexdigest()
        if label == 'after':
            suffix = '.exe' if os.name == 'nt' else ''
            shutil.copy2(binary_dir / ('vmpc-audio-benchmark' + suffix), package / ('vmpc-audio-benchmark' + suffix))
    before, after = manifest['builds']['before'], manifest['builds']['after']
    for key in ['compiler', 'config', 'pointer_bytes', 'cmake_options', 'effective_flags']:
        if before[key] != after[key]:
            raise RuntimeError('Unequal build setting: ' + key)
    if os.name == 'nt':
        audit_windows(package, root / 'after/build', manifest)
    (package / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    shutil.copy2(HERE / 'README.md', package / 'README.md')
    (package / 'run-benchmark.cmd').write_bytes(b'@echo off\r\ncd /d "%~dp0"\r\nvmpc-audio-benchmark.exe %*\r\necho.\r\npause\r\n')
    if args.smoke:
        executable = package / ('vmpc-audio-benchmark.exe' if os.name == 'nt' else 'vmpc-audio-benchmark')
        run(executable, '--self-test', cwd=package)
        run(executable, '--smoke', cwd=package)
    archive = shutil.make_archive(str(root / ('audio-benchmark-windows7-x86' if os.name == 'nt' else 'audio-benchmark-' + platform.system().lower() + '-' + platform.machine())), 'zip', package)
    print('Package:', args.output.resolve() / Path(archive).name)


def audit_windows(package, build, manifest):
    cache = (build / 'CMakeCache.txt').read_text()
    linker = next((line.split('=', 1)[1] for line in cache.splitlines() if line.startswith('CMAKE_LINKER:')), '')
    dumpbin = Path(linker).with_name('dumpbin.exe')
    if not dumpbin.exists():
        raise RuntimeError('Cannot find dumpbin for runtime dependency audit')
    system = {'kernel32.dll', 'user32.dll', 'gdi32.dll', 'winspool.drv', 'comdlg32.dll',
              'advapi32.dll', 'shell32.dll', 'ole32.dll', 'oleaut32.dll', 'uuid.dll',
              'odbc32.dll', 'odbccp32.dll', 'winmm.dll', 'version.dll', 'shlwapi.dll',
              'ws2_32.dll', 'imm32.dll', 'comctl32.dll', 'psapi.dll', 'setupapi.dll',
              'dwmapi.dll', 'uxtheme.dll', 'wininet.dll', 'crypt32.dll', 'secur32.dll',
              'bcrypt.dll', 'msimg32.dll', 'rpcrt4.dll', 'msacm32.dll',
              'dxgi.dll', 'dwrite.dll', 'd2d1.dll', 'vfw32.dll', 'avifil32.dll',
              'avicap32.dll', 'opengl32.dll', 'glu32.dll', 'dbghelp.dll',
              'uiautomationcore.dll'}
    import re
    for exe in package.glob('*.exe'):
        data = exe.read_bytes()
        pe = int.from_bytes(data[0x3c:0x40], 'little')
        if data[pe:pe+4] != b'PE\0\0' or int.from_bytes(data[pe+4:pe+6], 'little') != 0x14c:
            raise RuntimeError('Not an x86 PE executable: ' + str(exe))
        output = subprocess.check_output([str(dumpbin), '/dependents', str(exe)], text=True)
        imports = sorted(set(re.findall(r'^\s+([\w.-]+\.(?:dll|drv))\s*$', output, re.M | re.I)))
        unsupported = [x for x in imports if x.lower() not in system]
        if unsupported:
            raise RuntimeError('Unbundled/non-Win7 runtime dependencies: ' + ', '.join(unsupported))
        (package / (exe.stem + '-imports.txt')).write_text(subprocess.check_output([str(dumpbin), '/imports', str(exe)], text=True))
        manifest.setdefault('windows_imports', {})[exe.name] = imports


if __name__ == '__main__':
    main()
