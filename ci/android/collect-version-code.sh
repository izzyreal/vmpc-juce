#!/usr/bin/env sh
set -eu

bundle_path="android/app/build/outputs/bundle/release/app-release.aab"
code_path="dist/version-code-android.txt"

# Read the built artifact: another Gradle invocation would resolve a new code.
version_code="$(java -jar /opt/android-tools/bundletool.jar dump manifest \
    --bundle "$bundle_path" --xpath '/manifest/@android:versionCode')"

case "$version_code" in
    ''|*[!0-9]*)
        echo "Invalid versionCode in Android bundle: $version_code" >&2
        exit 1
        ;;
esac
if [ "${#version_code}" -gt 10 ] || [ "$version_code" -lt 1 ] || [ "$version_code" -gt 2100000000 ]; then
    echo "Android bundle versionCode is outside 1...2100000000: $version_code" >&2
    exit 1
fi

mkdir -p dist
printf '%s\n' "$version_code" > "$code_path"
echo "Android bundle versionCode: $version_code"
