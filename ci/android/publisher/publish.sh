#!/bin/sh
set +x
set -eu

: "${VMPC_GOOGLE_PLAY_SERVICE_ACCOUNT_JSON_BASE64:?VMPC_GOOGLE_PLAY_SERVICE_ACCOUNT_JSON_BASE64 is required}"

bundle_path="dist/VMPC2000XL-android-arm64-release-signed.aab"
version_path="dist/version-android.txt"

test -f "$bundle_path"
test -f "$version_path"

version="$(tr -d '\r\n' < "$version_path")"
test -n "$version"

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
