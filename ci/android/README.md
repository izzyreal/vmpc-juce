# Android CI image

The Android job uses a repository-owned, versioned toolchain image. Build it on
the Ciwi Docker host whenever `Dockerfile` changes:

```sh
./ci/android/build-image.sh
```

This creates `vmpc-android-ci:api36-ndk28.1-r1` with JDK 17, Android SDK and
build tools 36, NDK 28.1, CMake 3.28, Ninja, ccache, and bundletool. Increment
the final `rN` in both this directory and `ciwi-project.yaml` when rebuilding
the image with changed contents.
