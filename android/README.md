# Android build

The Android app is a Gradle packaging layer around the repository's existing
CMake project. Projucer is not part of the build.

## Prerequisites

- Android SDK 36
- Android SDK build tools
- Android NDK 28.1.13356709
- CMake 3.24 or newer and Ninja
- JDK 17 or 21 for running Gradle

Set `ANDROID_HOME` or create an untracked `local.properties` containing:

```properties
sdk.dir=/absolute/path/to/Android/sdk
# Set this when CMake is not installed through Android SDK Manager:
cmake.dir=/absolute/path/to/cmake/prefix
```

## Debug APK

```sh
cd android
./gradlew :app:assembleDebug
```

The APK is written below `app/build/outputs/apk/debug`.

## Play release bundle

Release signing details are read from environment variables or equivalently
named Gradle properties. Secrets must not be committed.

```sh
export VMPC_ANDROID_STORE_FILE=/absolute/path/to/upload-key.jks
export VMPC_ANDROID_STORE_PASSWORD=...
export VMPC_ANDROID_KEY_ALIAS=...
export VMPC_ANDROID_KEY_PASSWORD=...
./gradlew :app:bundleRelease
```

The AAB is written below `app/build/outputs/bundle/release`.

The initial build supports `arm64-v8a` and Android 7.0 (API 24) or newer.
Before publishing, verify audio latency and USB/Bluetooth MIDI on physical
hardware; the emulator cannot validate those properties.
