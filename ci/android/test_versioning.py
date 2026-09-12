"""Run with Python 3 and the same JAVA_HOME used by the Android Gradle build.

Uses a temporary, SDK-free Gradle project and fake publishing/container commands.
No APK/AAB is built and no network publishing takes place.
"""
import base64
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]


class VersioningTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="android-version-test-")
        self.addCleanup(self.temp.cleanup)
        self.work = Path(self.temp.name)
        self.env = os.environ.copy()
        self.env.pop("VMPC_ANDROID_VERSION_CODE", None)
        (self.work / "settings.gradle").write_text("rootProject.name = 'version-test'\n")
        # Load the real build logic; assertions exercise clock and range boundaries.
        script = str(ROOT / "android/version-code.gradle").replace("'", "\\'")
        (self.work / "build.gradle").write_text("""
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.TaskAction
apply from: '%s'
def loader = new GroovyClassLoader(getClass().classLoader)
loader.parseClass(new File('%s'))
def codeType = loader.loadClass('AndroidVersionCode')
assert codeType.fromEpochSeconds(1577836801L) == 1
assert codeType.fromEpochSeconds(1577845819L) == 9019
assert codeType.fromEpochSeconds(3677836800L) == 2100000000
for (seconds in [1577836800L, 3677836801L]) {
    try { codeType.fromEpochSeconds(seconds); assert false }
    catch (org.gradle.api.GradleException expected) {}
}
for (value in ['', ' ', '-1', '+1', '1.0', 'abc', '0', '2100000001', '999999999999999999999999']) {
    try { codeType.parse(value); assert false }
    catch (org.gradle.api.GradleException expected) {}
}
assert codeType.parse('0009019') == 9019
abstract class ReportCode extends DefaultTask {
    @Input abstract Property<Integer> getCode()
    @TaskAction void report() { println('RESOLVED_CODE=' + code.get()) }
}
tasks.register('reportCode', ReportCode) { code.set(vmpcAndroidVersionCode) }
""" % (script, script))

    def gradle(self, *args, env=None, success=True):
        result = subprocess.run(
            [str(ROOT / "android/gradlew"), "-p", str(self.work), "--offline",
             "--console=plain", "--configuration-cache", "reportCode", *args],
            env=env or self.env, text=True, capture_output=True,
        )
        output = result.stdout + result.stderr
        if success:
            self.assertEqual(result.returncode, 0, output)
        else:
            self.assertNotEqual(result.returncode, 0, output)
        return output

    def test_time_refreshes_with_configuration_cache(self):
        first = int(re.search(r"RESOLVED_CODE=(\d+)", self.gradle())[1])
        time.sleep(1.1)
        second = int(re.search(r"RESOLVED_CODE=(\d+)", self.gradle())[1])
        self.assertGreater(first, 9018)
        self.assertGreater(second, first)
        self.assertLessEqual(second, 2100000000)

    def test_override_precedence_validation_and_cache(self):
        self.assertIn("RESOLVED_CODE=12345", self.gradle("-PVMPC_ANDROID_VERSION_CODE=12345"))
        again = self.gradle("-PVMPC_ANDROID_VERSION_CODE=12345")
        self.assertIn("RESOLVED_CODE=12345", again)
        self.assertIn("Reusing configuration cache", again)
        env = dict(self.env, VMPC_ANDROID_VERSION_CODE="23456")
        self.assertIn("RESOLVED_CODE=23456", self.gradle("-PVMPC_ANDROID_VERSION_CODE=12345", env=env))
        env["VMPC_ANDROID_VERSION_CODE"] = ""
        self.assertIn("must be a decimal integer", self.gradle("-PVMPC_ANDROID_VERSION_CODE=12345", env=env, success=False))

    def mock(self, name, body):
        path = self.work / "bin" / name
        path.parent.mkdir(exist_ok=True)
        path.write_text("#!/bin/sh\nset -eu\n" + body)
        path.chmod(0o755)
        self.env["PATH"] = str(path.parent) + os.pathsep + os.environ["PATH"]

    def shell(self, path, success=True):
        result = subprocess.run(["sh", str(ROOT / path)], cwd=self.work,
                                env=self.env, capture_output=True, text=True)
        if success:
            self.assertEqual(result.returncode, 0, result.stderr)
        else:
            self.assertNotEqual(result.returncode, 0)
        return result

    def test_aab_metadata_and_publishing(self):
        self.mock("java", "printf '%s\\n' \"$@\" > java-args\nprintf '%s\\n' \"$TEST_CODE\"\n")
        self.env["TEST_CODE"] = "220000123"
        self.shell("ci/android/collect-version-code.sh")
        args = (self.work / "java-args").read_text()
        self.assertIn("dump\nmanifest\n", args)
        self.assertIn("/manifest/@android:versionCode", args)
        dist = self.work / "dist"
        self.assertEqual((dist / "version-code-android.txt").read_text(), "220000123\n")
        (dist / "version-android.txt").write_text("0.9.18\n")
        (dist / "VMPC2000XL-android-arm64-release-signed.aab").write_bytes(b"test bundle")
        self.mock("bundle", "printf '%s\\n' \"$@\" > publish-args\n")
        self.env["VMPC_GOOGLE_PLAY_SERVICE_ACCOUNT_JSON_BASE64"] = base64.b64encode(b"{}").decode()
        result = self.shell("ci/android/publisher/publish.sh")
        self.assertIn("0.9.18 (versionCode 220000123)", result.stdout)
        args = (self.work / "publish-args").read_text()
        self.assertIn("--track\ninternal\n", args)
        (self.work / "publish-args").unlink()
        for code in ["", "oops", "0", "2100000001"]:
            (dist / "version-code-android.txt").write_text(code)
            self.shell("ci/android/publisher/publish.sh", success=False)
            self.assertFalse((self.work / "publish-args").exists())
        (dist / "version-code-android.txt").unlink()
        self.shell("ci/android/publisher/publish.sh", success=False)
        for code in ["", "oops", "0", "2100000001", "999999999999999999999999"]:
            self.env["TEST_CODE"] = code
            self.shell("ci/android/collect-version-code.sh", success=False)
            self.assertFalse((dist / "version-code-android.txt").exists())

    def test_container_forwards_override_by_name(self):
        self.mock("docker", "printf '%s\\n' \"$@\" > docker-args\nprintf '%s' \"$VMPC_ANDROID_VERSION_CODE\" > code\n")
        self.env.update(CIWI_FETCHCONTENT_SOURCES_DIR=str(self.work), CCACHE_DIR=str(self.work),
                        GRADLE_USER_HOME=str(self.work), VMPC_ANDROID_VERSION_CODE="220000321")
        result = subprocess.run(["sh", str(ROOT / "ci/android/run-in-container.sh"), "true"],
                                cwd=self.work, env=self.env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("--env\nVMPC_ANDROID_VERSION_CODE\n", (self.work / "docker-args").read_text())
        self.assertEqual((self.work / "code").read_text(), "220000321")


if __name__ == "__main__":
    unittest.main(verbosity=2)
