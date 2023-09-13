# Sync Modes

Inputs are saving the FrameID at which they were set. We only send the frames that actually changed.

Users should use the frame data for On Start Frame events and On End Frame events.

# Input buffer limit parameter. 

In Independent and Delayed Synchronized modes, this will enqueue cooks until it reach this limit and the buffer is full. When the buffer is full, older cooks will be merged with the next cook so that some values are still being sent. More details can be found in the documentation in the “Inputs” section.

## Independent

Starts the cook and gets the output when the cook is ready. This is the default mode.

## Synchronized

In Synchronized mode, it is guaranteed that the whole Frame processing will happen within the same UE Tick, which might lead to a drop in UE FrameRate if the Cook takes too much time as we are forcing UE to wait.

## Delayed Synchronized

In Delayed Synchronized (and Independant) mode, a Frame is sent on Tick, but only one is under process at a given time. If the Cook is still under process, the next Frame is put in a queue. When the Cook is done, the next Frame in the queue starts processing. If the queue becomes too big (bigger than the Input Buffer Limit limit parameter in the TouchEngineComponent) the oldest set of inputs is discarded and we raise the On End Frame event with an error code Inputs Discarded.

If we have the following input queue:

- Frame 20: Set Pulse input `i/pulse` to true, and set Int input `i/int` to 10
- Frame 21: Set Float input `i/float` to 20.0
- Frame 22: Set Pulse input `i/pulse` to true, and set Float input `i/float` to 15

If Frame 20 gets dropped, its inputs will be given to the next set of inputs. So inputs for Frame 21 will look like this:

- Frame 21: Set Pulse input `i/pulse` to true, set Int input `i/int` to 10, Set Float input `i/float` to 20.0

If Frame 21 also gets dropped, its inputs will be given to the next set of inputs. So inputs for Frame 22 will look like this:

- Frame 22: Set Pulse input `i/pulse` to true, set Int input `i/int` to 10, Set Float input `i/float` to 15.0

Note that the value of `i/pulse` from frame 20 and the value of `i/float` from Frame 21 were discarded, as they were already going to be set by Frame 22. This also means that the Pulse input `i/pulse` will only be set once (the value set on Frame 20 ended up being discarded)
