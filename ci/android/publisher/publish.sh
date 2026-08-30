#!/bin/sh
set +x
set -eu

: "${VMPC_GOOGLE_PLAY_SERVICE_ACCOUNT_JSON_BASE64:?VMPC_GOOGLE_PLAY_SERVICE_ACCOUNT_JSON_BASE64 is required}"

bundle_path="dist/VMPC2000XL-android-arm64-release-signed.aab"
version_path="dist/version-android.txt"

require_nonempty_file() {
  if [ ! -f "$1" ]; then
    echo "Missing Android publishing input: $1" >&2
    return 1
  fi
  if [ ! -s "$1" ]; then
    echo "Empty Android publishing input: $1" >&2
    return 1
  fi
}

require_nonempty_file "$bundle_path"
require_nonempty_file "$version_path"

version="$(tr -d '\r\n' < "$version_path")"
if [ -z "$version" ]; then
  echo "Android publishing input contains no version: $version_path" >&2
  exit 1
fi

service_account_json="$(mktemp)"
cleanup() {
  rm -f "$service_account_json"
}
trap cleanup EXIT HUP INT TERM

printf '%s' "$VMPC_GOOGLE_PLAY_SERVICE_ACCOUNT_JSON_BASE64" \
  | base64 -d > "$service_account_json"

echo "Publishing VMPC2000XL ${version} to the Google Play internal track"

bundle exec fastlane supply \
  --aab "$bundle_path" \
  --json_key "$service_account_json" \
  --package_name nl.izmar.vmpc2000xl \
  --track internal \
  --release_status completed \
  --skip_upload_metadata true \
  --skip_upload_changelogs true \
  --skip_upload_images true \
  --skip_upload_screenshots true \
  --timeout 300
