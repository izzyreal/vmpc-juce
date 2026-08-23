#!/usr/bin/env sh
set -eu

if [ "$#" -eq 0 ]; then
    echo "Usage: $0 <command> [argument ...]" >&2
    exit 1
fi

: "${CIWI_FETCHCONTENT_SOURCES_DIR:?CIWI_FETCHCONTENT_SOURCES_DIR is required}"
: "${CCACHE_DIR:?CCACHE_DIR is required}"
: "${GRADLE_USER_HOME:?GRADLE_USER_HOME is required}"

image_tag="${VMPC_ANDROID_CI_IMAGE:-vmpc-android-ci:api36-ndk28.1-r6}"

exec docker run --rm \
    --user "$(id -u):$(id -g)" \
    --env HOME=/tmp \
    --env ANDROID_USER_HOME=/tmp/.android \
    --env JAVA_TOOL_OPTIONS=-Duser.home=/tmp \
    --env CIWI_FETCHCONTENT_SOURCES_DIR=/ciwi-fetchcontent \
    --env CCACHE_DIR=/ccache \
    --env CCACHE_BASEDIR=/work \
    --env CCACHE_NOHASHDIR=true \
    --env CCACHE_COMPILERCHECK=content \
    --env CMAKE_C_COMPILER_LAUNCHER=ccache \
    --env CMAKE_CXX_COMPILER_LAUNCHER=ccache \
    --env GRADLE_USER_HOME=/gradle-cache \
    --volume "$PWD:/work" \
    --volume "$CIWI_FETCHCONTENT_SOURCES_DIR:/ciwi-fetchcontent" \
    --volume "$CCACHE_DIR:/ccache" \
    --volume "$GRADLE_USER_HOME:/gradle-cache" \
    --workdir /work \
    "$image_tag" \
    "$@"
