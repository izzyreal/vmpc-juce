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

## Release topology

There are two Android release paths:

- `android` -> `sign-android` -> `release-android-focused` is the focused path
  for building and publishing only Android.
- `build` -> `sign-android-build` -> `release-android` is the Android branch of
  the full cross-platform release. `release-github` waits for both
  `release-ios` and `release-android`, so the version is bumped only after both
  app-store submissions have succeeded.

The two signing pipelines share one job definition, as do the two publishing
pipelines. The separate pipeline IDs are needed because CIWI artifact sources
must name their direct upstream pipeline. The full release reuses the Android
artifact already produced by `build`; it does not run a second Android build.

## Android build numbers

Gradle automatically assigns a `versionCode` using whole UTC seconds since
2020-01-01, independently of the version name in `VERSION`. To override it,
set `VMPC_ANDROID_VERSION_CODE` on the Android build job; the container wrapper
forwards it to Gradle. See `android/README.md` for validation and clock limits.

After building, `collect-version-code.sh` reads the code from the actual AAB
manifest using bundletool and writes `dist/version-code-android.txt`. The build
and signing jobs both retain this artifact alongside `dist/version-android.txt`.
Signing preserves the embedded code, and publishing logs the version name and
code from those artifacts. Neither stage generates a new code.

To publish another internal-testing binary with the same visible version,
rerun the Android build, signing, and publishing chain with `VERSION` unchanged.
Retrying only publishing with the already-uploaded AAB reuses its code and will
still be rejected. Upload builds in increasing code order. This behavior does
not alter the full release pipeline's existing semantic-version auto-bump.

Run the focused versioning checks with Python 3 and the Android build's JDK:

```sh
python3 ci/android/test_versioning.py
```

These use a temporary SDK-free Gradle project, exercise configuration-cache
behavior, and mock bundletool, Docker, and Fastlane. They do not build Android
binaries or contact Play. The Gradle wrapper distribution must already be cached
because the checks run Gradle offline.

## Release signing

The build pipelines remain credential-free and publish an unsigned release
bundle. Their corresponding signing pipeline publishes
`VMPC2000XL-android-arm64-release-signed.aab`. Signing runs in a pinned JDK 17
job container.

The signing job reads these fields from `google-play` in the `kv` mount of the
`home-vault` connection:

- `upload-keystore-base64`
- `upload-keystore-password`
- `upload-key-password`

The key alias and expected certificate fingerprint are fixed in
`ciwi-project.yaml`. The decoded keystore exists only in a temporary file inside
the signing container and is removed when the step exits.

## Google Play publishing

The publishing jobs upload the signed AAB to the `internal` Google Play track
with release status `completed`. They use Fastlane `supply`, pinned by both the
publisher image and `Gemfile.lock`. Build the publisher image manually with:

```sh
./ci/android/publisher/build-image.sh
```

This creates `vmpc-android-publisher-ci:fastlane-2.235.0-r1`. The image is built
by the job rather than assumed to exist on the CI agent; the agent only needs
Git and Docker. The image uses a digest-pinned Ruby base, and its build-only
compiler toolchain is excluded from the runtime stage.

The publishing job reads `publisher-service-account-json-base64` from the same
`google-play` Vault record. The decoded JSON key exists only in a temporary file
inside the publisher container and is removed when Fastlane exits.

The Google Play service account needs access to VMPC2000XL and permission to
view app information and manage releases on testing tracks. It does not need
production-release or tester-management permission. Testers and testing groups
remain managed in Play Console.
