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
The debug application id is `nl.izmar.vmpc2000xl.debug`, so it can be installed
alongside the release app.

## Play release bundle

Release builds are unsigned by default, which is useful for CI validation.
Signing details are read from environment variables or equivalently named
Gradle properties. If one is specified, all four are required. Secrets must not
be committed.

```sh
export VMPC_ANDROID_STORE_FILE=/absolute/path/to/upload-key.jks
export VMPC_ANDROID_STORE_PASSWORD=...
export VMPC_ANDROID_KEY_ALIAS=...
export VMPC_ANDROID_KEY_PASSWORD=...
./gradlew :app:bundleRelease
```

The AAB is written below `app/build/outputs/bundle/release`.

## Version names and build numbers

The user-facing release `versionName` comes from the root `VERSION` file.
The integer `versionCode` is generated once per Gradle invocation from whole
UTC seconds since 2020-01-01. Multiple builds can therefore share the same
visible version, such as `0.9.18`, while having different Play build numbers.
Gradle prints both values during configuration. Debug builds retain the
`-debug` version-name suffix.

To select a code explicitly, set `VMPC_ANDROID_VERSION_CODE` in the environment
or pass a Gradle property:

```sh
./gradlew :app:bundleRelease -PVMPC_ANDROID_VERSION_CODE=220000000
```

The environment variable takes precedence, including when it is empty (which
is an error). Explicit codes must contain only decimal digits and be in
`1...2100000000`. For an upload, choose a code higher than the highest one
already accepted by Play; the example above is not a permanently safe value.
An explicit override also allows reproducing a build with the same code.

Automatic numbering depends on an accurate system clock. Builds started in
the same second can share a code. Rebuild in a later second or supply a higher
override in that case, and upload builds in increasing code order. No Play API
query or shared counter is involved.

Once Play has accepted an AAB's code, another upload needs a newly built AAB.
Changing the code at the signing or publishing stage is not supported.
You can keep `VERSION` unchanged when rebuilding for internal testing.

## Emulator keyboard input

Set `hw.keyboard=yes` in the AVD configuration and start the emulator with raw
Qt keyboard forwarding. The flag avoids the emulator's synthesized modifier
repeats, and is required for representative VMPC keyboard behavior:

```sh
$ANDROID_HOME/emulator/emulator -avd YOUR_AVD -feature QtRawKeyboardInput
```

The initial build supports `arm64-v8a` and Android 7.0 (API 24) or newer.
Before publishing, verify audio latency and USB/Bluetooth MIDI on physical
hardware; the emulator cannot validate those properties.
