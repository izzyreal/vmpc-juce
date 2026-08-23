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
