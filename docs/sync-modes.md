# Sync Modes
A `FrameID` is used to uniquely identify a set of inputs and their matching outputs. Inputs and Parameters have their `FrameID` automatically set at the time the node `Set TouchEngine Input/Parameter` is called. To be able to match a set of inputs with their outputs, users should use the `Frame Data` for `On Start Frame` events and `On End Frame` events. 

For a given cook, we only send the inputs for which the node `Set TouchEngine Input/Parameter` was called.

> ⚠️ For input Textures, you need to explicitly call Set TouchEngine Input for every frame where you want the texture copied. When you call the node, the texture will end up being copied and sent to TE and as we are not sending the inputs every frame, we need to call the node again to start copying the next frame. The moment Set TouchEngine Input is the moment the texture is copied.

## Cook Modes: Independent, Synchronized and Delayed Synchronized
The main difference between these modes relates to the time mode of TouchEngine, and if we want to stall Unreal Engine until we receive the outputs.

In Independent mode, TouchEngine continues running in the background whereas in Synchronized and Delayed Synchronized modes, TouchEngine follows the time used in Unreal Engine to keep it synchronized with our component.

In Synchronized mode only, we stall Unreal Engine to wait for the cook from TouchEngine to be done.

### Independent
Starts the cook and gets the outputs when the cook is ready. TouchEngine runs in the background. This is the default mode.

### Delayed Synchronized
Starts the cook and gets the outputs when the cook is ready. TouchEngine’s internal time is linked to Unreal’s.

### Synchronized
 Starts the cook and waits for the outputs before the frame completes. TouchEngine’s internal time is linked to Unreal’s.

In Synchronized mode, it is guaranteed that the whole Frame processing will happen within the same UE Tick, which might lead to a drop in UE frame rate if the Cook takes too much time, as we are forcing UE to wait. This also means that in Synchronized mode, a call to `On Start Frame` will always be directly followed by a call to `On End Frame`, before the next frame starts. In other cook modes, there might be multiple calls to `On Start Frame` before receiving a call to On End Frame

## Input buffer limit parameter
In Independent and Delayed Synchronized modes, a Frame is sent on Tick, but only one is under process at a given time. The cooks will be enqueued until the current cook is done when the next Frame in the queue starts processing. There is a limit to the size of this queue defined by the Input Buffer Limit parameter. When the buffer is full, older cooks will be merged with the next cook so that some values are still being sent.  The On End Frame event will be raised with an error code Inputs Discarded. More details can be found in the documentation in the “Inputs” section.

If we have the following input queue:

* Frame 20: Set Pulse input `i/pulse` to true, and set Int input `i/int` to 10
* Frame 21: Set Float input `i/float` to 20.0
* Frame 22: Set Pulse input `i/pulse` to true, and set Float input `i/float` to 15

If Frame 20 gets dropped, its inputs will be given to the next set of inputs. So inputs for Frame 21 will look like this:

* Frame 21: Set Pulse input `i/pulse` to true, set Int input `i/int` to 10, Set Float input `i/float` to 20.0

If Frame 21 also gets dropped, its inputs will be given to the next set of inputs. So inputs for Frame 22 will look like this:

* Frame 22: Set Pulse input `i/pulse` to true, set Int input `i/int` to 10, Set Float input `i/float` to 15.0

Note that the value of `i/pulse` from frame 20 and the value of `i/float` from Frame 21 were discarded, as they were already being set by Frame 22. This also means that the Pulse input `i/pulse` will only be set once (the value set on Frame 20 ends up being discarded)
