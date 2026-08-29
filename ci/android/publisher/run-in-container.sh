#!/bin/sh
set -eu

: "${VMPC_GOOGLE_PLAY_SERVICE_ACCOUNT_JSON_BASE64:?VMPC_GOOGLE_PLAY_SERVICE_ACCOUNT_JSON_BASE64 is required}"

IMAGE="vmpc-android-publisher-ci:fastlane-2.235.0-r1"
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"

exec docker run --rm \
  --env FASTLANE_OPT_OUT_USAGE=1 \
  --env FASTLANE_SKIP_UPDATE_CHECK=1 \
  --env HOME=/tmp \
  --env LANG=C.UTF-8 \
  --env LC_ALL=C.UTF-8 \
  --env VMPC_GOOGLE_PLAY_SERVICE_ACCOUNT_JSON_BASE64 \
  --volume "$ROOT:/work" \
  --workdir /work \
  "$IMAGE" \
  "$@"
