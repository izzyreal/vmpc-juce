## Plugin Compatibility Matrix

VST instruments with audio inputs are a bit of a rare one. Once upon a time a sound generating plugin was thought to never need audio inputs. Only effects needed ins and outs.

Long story short, using VMPC2000XL as a sampler, recording new sounds with it, doesn't work properly in all DAWs. Maybe some things will improve as my understanding of it does, but for now the below matrices apply.

| Linux  | LV2 Audio | LV2 MIDI | VST3 Audio | VST3 MIDI |
| :----: | :-------: | :------: | :--------: | :-------: |
| Carla  |     ?     |    ?     |     ?      |     ?     |
|  LMMS  |     ?     |    ?     |     ?      |     ?     |
| Ardour |     ?     |    ?     |     ?      |     ?     |




|     Windows     |                    VST3 Audio                     |             VST3 MIDI             |
| :-------------: | :-----------------------------------------------: | :-------------------------------: |
| Ableton Live 10 |           1 stereo in<br />5 stereo out           | 16 channels in<br />1 channel out |
|    Reaper 6     | 1 stereo in (always monitoring)<br />5 stereo out |                                   |
|    Cubase 10    |           0 stereo in<br />5 stereo out           |                                   |



|       Mac       |                              AU                              | VST3 |
| :-------------: | :----------------------------------------------------------: | :--: |
|   Logix Pro X   | 5 stereo out<br />1 stereo in (Side Chain in top right of plugin) |      |
| Ableton Live 10 |                5 stereo out<br />1 stereo in                 |      |
|    Reaper 6     |                                                              |      |
|    Cubase 10    |                                                              |      |

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
