FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG ANDROID_COMMAND_LINE_TOOLS=13114758
ARG ANDROID_PLATFORM=36
ARG ANDROID_BUILD_TOOLS=36.0.0
ARG ANDROID_NDK=28.1.13356709
ARG ANDROID_CMAKE=3.28.3
ARG BUNDLETOOL_VERSION=1.18.2

ENV ANDROID_HOME=/opt/android-sdk \
    ANDROID_SDK_ROOT=/opt/android-sdk \
    PATH=/opt/android-sdk/cmdline-tools/latest/bin:/opt/android-sdk/platform-tools:/opt/android-sdk/build-tools/36.0.0:/opt/android-sdk/cmake/3.28.3/bin:$PATH \
    LANG=C.UTF-8

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates ccache curl git openjdk-17-jdk-headless ninja-build \
        unzip zip \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p ${ANDROID_HOME}/cmdline-tools /opt/android-tools \
    && curl -fL --retry 5 \
        https://dl.google.com/android/repository/commandlinetools-linux-${ANDROID_COMMAND_LINE_TOOLS}_latest.zip \
        -o /tmp/android-command-line-tools.zip \
    && unzip -q /tmp/android-command-line-tools.zip -d /tmp/android-cli \
    && mv /tmp/android-cli/cmdline-tools ${ANDROID_HOME}/cmdline-tools/latest \
    && rm -rf /tmp/android-command-line-tools.zip /tmp/android-cli \
    && yes | sdkmanager --licenses >/dev/null \
    && sdkmanager \
        "platform-tools" \
        "platforms;android-${ANDROID_PLATFORM}" \
        "build-tools;${ANDROID_BUILD_TOOLS}" \
        "ndk;${ANDROID_NDK}" \
        "cmake;${ANDROID_CMAKE}" \
    && curl -fL --retry 5 \
        https://github.com/google/bundletool/releases/download/${BUNDLETOOL_VERSION}/bundletool-all-${BUNDLETOOL_VERSION}.jar \
        -o /opt/android-tools/bundletool.jar \
    && rm -rf /root/.android/cache /root/.cache

RUN java -version \
    && sdkmanager --version \
    && cmake --version \
    && ninja --version
