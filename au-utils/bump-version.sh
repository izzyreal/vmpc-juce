#!/bin/sh

PLIST_FILE="../build-xcode/CMakeFiles/vmpc2000xl_AU.dir/Info.plist"

CURRENT_VERSION=$(/usr/libexec/PlistBuddy -c "Print :AudioComponents:0:version" "$PLIST_FILE")
NEW_VERSION=$((CURRENT_VERSION + 1))

/usr/libexec/PlistBuddy -c "Set :AudioComponents:0:version $NEW_VERSION" "$PLIST_FILE"

echo "Updated version component 0: $NEW_VERSION"

CURRENT_VERSION=$(/usr/libexec/PlistBuddy -c "Print :AudioComponents:1:version" "$PLIST_FILE")
NEW_VERSION=$((CURRENT_VERSION + 1))

/usr/libexec/PlistBuddy -c "Set :AudioComponents:1:version $NEW_VERSION" "$PLIST_FILE"

echo "Updated version component 1: $NEW_VERSION"

