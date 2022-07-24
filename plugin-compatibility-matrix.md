## Plugin Compatibility Matrix

VST instruments with audio inputs are a bit of a rare one. Once upon a time a sound generating plugin was thought to never need audio inputs. Only effects needed ins and outs.

Long story short, using VMPC2000XL as a sampler, recording new sounds with it, doesn't work properly in all DAWs. Maybe some things will improve as my understanding of it does, but for now the below matrices apply.

| Linux  | LV2 Audio | LV2 MIDI | VST3 Audio | VST3 MIDI |
| :----: | :-------: | :------: | :--------: | :-------: |
| Carla  | 2 mono in<br/>10 mono out | 16 channels in<br/>16 channels out | plugin opens but<br/>Carla crashes | plugin opens but<br/>Carla crashes |
|  LMMS  | no LV2 support | no LV2 support | no VST3 support<br/>in stable | no VST3 support<br/>in stable |
| Ardour | plugin doesn't open<br/>don't know why | plugin doesn't open<br/>don't know why | plugin opens but<br/>Ardour crashes | plugin opens but<br/>Ardour crashes |
| Reaper |?|?|?|?|
| Bitwig |?|?|?|?|
| Qtractor |?|?|?|?|
| Mixxx |?|?|?|?|
| [AudioPluginHost](https://github.com/juce-framework/JUCE/blob/master/extras/AudioPluginHost/AudioPluginHost.jucer) |?|?|?|?|


|   Windows 10    |                    VST3 Audio                     |             VST3 MIDI             |
| :-------------: | :-----------------------------------------------: | :-------------------------------: |
| Ableton Live 10 |           1 stereo in<br />5 stereo out           | 16 channels in<br />1 channel out |
|    Reaper 6     | 1 stereo in (always monitoring)<br />5 stereo out |                                   |
|    Cubase 10    |           0 stereo in<br />5 stereo out           |                                   |



|      macOS      |                              AU                              |                        AUv3                        | VST3 |
| :-------------: | :----------------------------------------------------------: | :------------------------------------------------: | ---- |
|  Logix Pro (X)  | 1 stereo in (Side Chain in top right of plugin)<br />5 stereo out | 0 stereo in<br />1 stereo out<br />only MIDI input |      |
| Ableton Live 10 |                1 stereo in<br />5 stereo out                 |                unsupported by Live                 |      |
|   GarageBand    |                                                              | 0 stereo in<br />1 stereo out<br />only MIDI input |      |
|    Cubase 10    |                                                              |                                                    |      |
|    Reaper 6     |                                                              |                                                    |      |

**Supported features**

* MIDI note on/off input and output
* Multiple instances
* MIDI CLOCK slave synchronization
* Any MIDI Controller (1-127) controls the slider
* Persist full state including samples to DAW projects and programs

**Not implemented**:

* Musical position synchronization
* Button to MIDI mapping

This will also serve as a rough indication of VMPC2000XL's MIDI and other control protocol support for the time being, which indeed is very crude and limited compared to the original.
