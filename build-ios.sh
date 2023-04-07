#!/bin/sh

cmake -B build -G Xcode -D CMAKE_SYSTEM_NAME="iOS"
echo 'Your iOS build is ready. Open vmpc-juce/build/vmpc2000xl.xcodeproj in Xcode and select the vmpc2000xl_Standalone target. If it's not there, add it via Manage Schemes.'
