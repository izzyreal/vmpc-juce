## Plugin Compatibility Matrix

VST instruments with audio inputs are a bit of a rare one. Once upon a time a sound generating plugin was thought to never need audio inputs. Only effects needed ins and outs.

Long story short, using VMPC2000XL as a sampler, recording new sounds with it, doesn't work properly in all DAWs. Maybe some things will improve as my understanding of it does, but for now the below matrices apply.

| Linux  | LV2 Audio | LV2 MIDI | VST3 Audio | VST3 MIDI |
| :----: | :-------: | :------: | :--------: | :-------: |
| Carla  | 2 mono in<br/>10 mono out | 16 channels in<br/>16 channels out | plugin opens but<br/>Carla crashes | plugin opens but<br/>Carla crashes |
|  LMMS  | no LV2 support | no LV2 support | no VST3 support<br/>in stable | no VST3 support<br/>in stable |
| Ardour | plugin doesn't open<br/>don't know why | plugin doesn't open<br/>don't know why | plugin opens but<br/>Ardour crashes | plugin opens but<br/>Ardour crashes |
| Reaper |?|?|?|?|
| Bitwig | no LV2 support | no LV2 support |?|?|
| Qtractor |?|?|?|?|
| Mixxx |?|?|?|?|
| [AudioPluginHost](https://github.com/juce-framework/JUCE/blob/master/extras/AudioPluginHost/AudioPluginHost.jucer) |?|?|?|?|


|   Windows 10    |                    VST3 Audio                     |             VST3 MIDI             |
| :-------------: | :-----------------------------------------------: | :-------------------------------: |
| Ableton Live 10 |           1 stereo in<br />5 stereo out           | 16 channels in<br />1 channel out |
|    Reaper 6     | 1 stereo in (always monitoring)<br />5 stereo out |                                   |
|    Cubase 10    |           0 stereo in<br />5 stereo out           |                                   |



|      macOS      |                              AU                              |                        AUv3                        | VST3                                                   |
| :-------------: | :----------------------------------------------------------: | :------------------------------------------------: | ------------------------------------------------------ |
|  Logic Pro (X)  | 1 stereo in (Side Chain in top right of plugin)<br />5 stereo out | 0 stereo in<br />1 stereo out<br />only MIDI input |                                                        |
| Ableton Live 10 |                1 stereo in<br />5 stereo out                 |                unsupported by Live                 |                                                        |
|   GarageBand    |                                                              | 0 stereo in<br />1 stereo out<br />only MIDI input |                                                        |
|    Cubase 14    |                         Unsupported                          |                    Unsupported                     | 5 stereo out, 10 mono out<br />1 stereo in,  2 mono in |
|    Reaper 6     |                                                              |                                                    |                                                        |

**Supported features**

* MIDI note on/off input and output
* Multiple instances
* MIDI CLOCK slave synchronization
* Any MIDI Controller (1-127) controls the slider
* Persist full state including samples to DAW projects and programs

**Not implemented**

* Musical position synchronization
* Button to MIDI mapping

This will also serve as a rough indication of VMPC2000XL's MIDI and other control protocol support for the time being, which indeed is very crude and limited compared to the original.

### Some notes on Logic
* Side-chain options may be hidden depending on your Logic settings. To see the side-chain option, enable "Show Advanced Tools" in Preferences > Advanced, and make sure the "Audio" checkbox is enabled.
* You can resize the window, but not via the plugin window's handle. Use the bottom-right of the background of VMPC2000XL itself instead. I've been informed this is a known issue with Logic, and that literally only Apple's own plugins allow that kind of resizing.
* I have seen occasional absence of keyboard input. This seems to happen due to some issue related to window focus. Close and reopen the plugin window to work around this.
* Logic seems to block key repeat for certain keys. The `+` and `-` keys that are by default used to turn the DATA wheel do not repeat. This can be annoying, but I don't have a workaround for now. Be aware, though, that as usual Shift and Control can be used for bigger increments. Press them both for even bigger increments.

### Some notes on Reaper

Enable "Send all keyboard input to plug-in" to use VMPC2000XL's full keyboard mapping. This is recommended, else basic keyboard input, like Esc to go to MAIN screen will close the plugin editor, and using F1-F6 keys is not possible.
<img width="579" alt="image" src="https://github.com/izzyreal/vmpc-juce/assets/3707432/4f90842e-e05f-4948-9662-5be69adb2749">

### Some notes on AUv3

VMPC2000XL is exposed as both an AUv3 [music device/`aumu`](https://developer.apple.com/documentation/audiotoolbox/1584142-audio_unit_types/kaudiounittype_musicdevice), as well as an AUv3 [music effect/`aumf`](https://developer.apple.com/documentation/audiotoolbox/1584142-audio_unit_types/kaudiounittype_musiceffect).

An AUv3 music device supports:
* MIDI input
* audio output

An AUv3 music effect supports:
* MIDI input
* audio output
* audio input

In order to use VMPC2000XL AUv3 as a sampler, you _have_ to insert it as an effect on the channel you want to sample. This channel you want to sample can be a mix bus to which some audio source is sent, or it can be directly on a channel that has an audio source like an instrument plugin.

Below is the mix bus approach used on a scenario where VMPC2000XL samples audio coming from Model D:

<img width="579" alt="image" src="https://github.com/izzyreal/vmpc-juce/assets/3707432/e71271cd-6049-4073-8603-cb4beaaa36ca">
