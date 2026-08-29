#!/bin/sh
set -eu

IMAGE="vmpc-android-publisher-ci:fastlane-2.235.0-r1"
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"

exec docker build \
  --pull \
  --tag "$IMAGE" \
  --file "$ROOT/ci/android/publisher/Dockerfile" \
  "$ROOT"
