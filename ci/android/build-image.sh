#!/usr/bin/env sh
set -eu

image_tag="${1:-vmpc-android-ci:api36-ndk28.1-r1}"
docker build --pull --tag "$image_tag" --file ci/android/Dockerfile .
