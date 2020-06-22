## Plugin Compatibility Matrix

VST instruments with audio inputs are a bit of a rare one. Once upon a time a sound generating plugin was thought to never need audio inputs. Only effects needed ins and outs.

Long story short, using vMPC2000XL as a sampler, recording new sounds with it, doesn't work properly in all DAWs. Maybe some things will improve as my understanding of it does, but for now the below matrix applies.

Multipe instances is currently not supported!


|     Windows     |                      VST2                      |                       VST3                        |
| :-------------: | :--------------------------------------------: | :-----------------------------------------------: |
| Ableton Live 10 |         1 stereo in<br />5 stereo out          |           1 stereo in<br />5 stereo out           |
|    Reaper 6     | 1 stereo (always monitoring)<br />5 stereo out | 1 stereo in (always monitoring)<br />5 stereo out |
|    Cubase 10    |         0 stereo in<br />5 stereo out          |           0 stereo in<br />5 stereo out           |

|       Mac       |                              AU                              | VST2 | VST3 |
| :-------------: | :----------------------------------------------------------: | :--: | :--: |
|   Logix Pro X   | 5 stereo out<br />1 stereo in (Side Chain in top right of plugin) |      |      |
| Ableton Live 10 |                5 stereo out<br />1 stereo in                 |      |      |
|    Reaper 6     |                                                              |      |      |
|    Cubase 10    |                                                              |      |      |

