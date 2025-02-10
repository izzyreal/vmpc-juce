#!/bin/sh
rm -rf ~/Library/Audio/Plug-Ins/Components/VMPC2000XL.component
rm -rf /Applications/VMPC2000XL.app
rm -rf ~/Library/Caches/AudioUnitCache
rm -rf ~/Library/Preferences/com.apple.audio.InfoHelper.plist
killall -9 AudioComponentRegistrar

