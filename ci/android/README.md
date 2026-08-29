# Android CI image

The Android jobs build the repository-owned, versioned toolchain image before
running the Gradle build. Docker's layer cache makes subsequent builds
incremental. To build the image manually for local testing or troubleshooting:

```sh
./ci/android/build-image.sh
```

This creates `vmpc-android-ci:api36-ndk28.1-r6` with a Linux host C/C++ toolchain,
the FreeType/X11 headers needed by JUCE's host tools, pkg-config, JDK 17, Android
SDK platform 36, build tools 35, NDK 28.1, CMake 3.30, Ninja, ccache, and
bundletool. Increment the final `rN` in both Android CI scripts when changing the
image contents.

## Release signing

The `android` pipeline remains credential-free and publishes an unsigned release
bundle. The manual, source-independent `sign-android` pipeline consumes the AAB
from the latest successful `android` run and publishes
`VMPC2000XL-android-arm64-release-signed.aab`. Signing runs in a pinned JDK 17
job container, so it can process an artifact produced by an older source
revision without depending on release tooling from that revision.

The signing job reads these fields from `google-play` in the `kv` mount of the
`home-vault` connection:

- `upload-keystore-base64`
- `upload-keystore-password`
- `upload-key-password`

The key alias and expected certificate fingerprint are fixed in
`ciwi-project.yaml`. The decoded keystore exists only in a temporary file inside
the signing container and is removed when the step exits.
