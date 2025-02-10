#!/bin/bash
./refresh-au.sh
./bump-version.sh

XCODE_PROJECT="../build-xcode/vmpc2000xl.xcodeproj"
XCODE_TARGET="vmpc2000xl_AU"

AU_TYPE="aumu"
AU_SUBTYPE="2kXa"
AU_MANUFACTURER="Izmr"

sleep 1

xcodebuild \
    -project $XCODE_PROJECT \
    -scheme $XCODE_TARGET \
    -destination 'platform=OS X,arch=arm64'

sleep 1

auval -v $AU_TYPE $AU_SUBTYPE $AU_MANUFACTURER

sleep 1

./refresh-au.sh

sleep 1

xcodebuild \
    -project $XCODE_PROJECT \
    -scheme $XCODE_TARGET \
    -destination 'platform=OS X,arch=arm64'

sleep 1

auval -v $AU_TYPE $AU_SUBTYPE $AU_MANUFACTURER

