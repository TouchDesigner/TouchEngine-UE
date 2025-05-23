# Main features

## Details Panel
![assets/main-features/touchengine_component_panel.png?raw=true](assets/main-features/touchengine_component_panel.png?raw=true)

The details panel is your go-to for the basic setup of the TouchEngine Component.

* Tox Asset: A property that can receive a ToxAsset, a .uasset pointing to a .tox file.
* Reload Tox: Forces the .tox to reload in the TouchEngine instance. This is useful if you changed parameters, inputs... etc.
* Allow running in editor: When Allow Running in Editor is toggled on, the TouchEngine is loaded with the .tox and can be used without the project running as packaged or in PIE mode. In blueprint, developers should make use of the TouchEngine Component Begin Play and End Play events. See [How To: Work In Editor 🔗](how-tos/work-in-editor.md).
* Load on begin play: In PIE or packaged mode, the TouchEngine will start and load the .tox as soon as the play event is called.
* Cook Mode: See [Sync Modes 🔗](sync-modes.md)
* TE Frame Rate: The frame rate at which the TouchEngine subprocess should be running.
* Input Buffer Limit: In Independent and Delayed Synchronized modes, this will enqueue cooks until it reach this limit and the buffer is full. When the buffer is full, older cooks will be merged with the next cook so that some values are still being sent. More details can be found in the documentation in the “Inputs” section.
* Component Settings: This is the section where all the Unreal properties specific to the currently loaded .tox are dynamically created. They are sorted in subsections, Parameters, Inputs and Outputs. Parameters are accessed with the nodes Get / Set TouchEngine Parameters, while Inputs and Outputs are accessed using Set TouchEngine Input and Get TouchEngine Output nodes respectively.
* Advanced
    * Pause on End Frame can be used to pause the editor when a frame was processed. This is only useful for debugging and it is only supported in Editor mode.
    * Exported / Imported Texture Pool Size properties were added for users to control the size of the texture pool used by TOP inputs and outputs in Unreal Engine. A texture pool is used to store temporary texture data while exchanging textures between the TouchEngine process and Unreal Engine. They are allocated and initialized once when the TouchEngine Component is first loaded.
    * Tox Load Timeout: The number of seconds to wait for the .tox to load in the TouchEngine before aborting.
    * Cook Timeout: The number of seconds to wait for a cook before cancelling it. If the cook is not done by that time, the component will raise a TouchEngineCookTimeout error and will continue running. Be cautious of not using too high values in Synchronized mode as we are stalling the GameThread, the application could become unusable.

## Events
![assets/main-features/touchengine_component_events.png?raw=true](assets/main-features/touchengine_component_events.png?raw=true)

- On Tox Started Loading
    - Called as soon as the TouchEngine starts loading the .tox file.
- On Tox Loaded
    - Called when the TouchEngine instance loads the .tox file. 
- On Tox Reset
    - Called when the TouchEngine instance is reset, and data is cleared.
- On Tox Failed Load
    - Called when the TouchEngine instance failed to load the .tox file.    
    - Error Message: A reason describing why the tox failed to load.
- On Tox Unloaded
    - Called when the TouchEngine instance unloads the .tox file.
    - It might be called multiple times and it is also called after On Tox Failed Load.
- On Start Frame
    - Called before sending the inputs to the TouchEngine.    
    - Frame Data: 
        * Frame Data Frame ID: The frame identifier, which is unique for this component, until it is restarted.
- On End Frame
    - Called after receiving the outputs from the TouchEngine.
    - Is Successful: True when Result is Success and the frame wasn’t dropped (Frame Data Was Frame Dropped)
    - Result: (Use a “switch” node)
        - Success
        - Inputs Discarded
        - Internal TouchEngine Error
        - Cancelled
        - Bad Request
        - Failed To Start Cook    
    - Frame Data: 
        - Frame ID: The frame identifier, which is unique for this component, until it is restarted.
        - Tick Latency: The number of ticks it took since On Start Frame was last called.
        - Latency: The number of milliseconds it took since On Start Frame was last called.
        - Was Frame Dropped: When Unreal runs faster than the TouchEngine sub process, it can happen that the frame gets dropped, although the cook happened and be successful. In that case, the Output wouldn’t have updated.
        - Frame Last Updated: The frame identifier of the last frame we received updated data from TouchEngine.
        - Cook Start Time: The internal start time of this cook returned by TouchEngine.
        - Cook End Time: The internal end time of this cook returned by TouchEngine.
- Begin Play
    - Begin Play for the Component.
    - Different from the Blueprint Begin Play, as it also fires In Editor.
    - When working in both Editor and PIE, this event should be preferred.
- End Play
    - End Play for the Component.
    - Different from the Blueprint End Play, as it also fires In Editor.
    - When working in both Editor and PIE, this event should be preferred.
- On Component Activated
    - Called when a Component has been activated, with parameter indicating if it was from a Reset.
    - Component: The Component Actor Object
    - Reset: Whether the Component Activated event triggered following a Reset.
- On Component Deactivated
    - Called when a Component has been deactivated.
    - Component: The Component Actor Object

## Nodes
![assets/main-features/touchengine_nodes01.png?raw=true](assets/main-features/touchengine_nodes01.png?raw=true)
![assets/main-features/touchengine_nodes02.png?raw=true](assets/main-features/touchengine_nodes02.png?raw=true)
![assets/main-features/touchengine_nodes02.png?raw=true](assets/main-features/touchengine_nodes03.png?raw=true)
![assets/main-features/touchengine_nodes02.png?raw=true](assets/main-features/touchengine_nodes04.png?raw=true)

### Interacting with component
#### States
- Start TouchEngine: Start and run the TouchEngine instance, will call ToxLoaded or ToxFailedLoad callbacks depending on the result.
- Can Start: Should UI, players, or other means, be allowed to start the TouchEngine.
- Is Running: Is the TouchEngine currently running.
- Load Tox: Load or reload the currently loaded .tox file.
- Stop TouchEngine: Stops and delete the TouchEngine instance.
#### Properties
- Get TouchEngine Input: Get the current object set for a TouchEngine Input.
- Set TouchEngine Input: Set the object to be passed from Unreal to the TouchEngine, to a given Input. Must be of a valid format for CHOP, DAT or TOP. For TOPs, the textures are copied the moment they are assigned using Set TouchEngine Input.
- Get TouchEngine Output: Get the object sent out of the TouchEngine and received by Unreal. Can be a CHOP Struct, a DAT Object, or a texture2D (TOP).
- Set TouchEngine Parameter: Set the value to be passed to a TouchEngine parameter at the blueprint level. Note that a parameter changed through the details panel will be sent to the TouchEngine as well.
- Get TouchEngine Parameter: Get the current value set for a TouchEngine Parameter.
- Get TouchEngine Component: Returns the TouchEngine Component assigned to a TouchEngine Actor.
### CHOPs
- Get Channel: Return a CHOP Channel Struct, based on a given Channel index.
- Get Channel by Name: Return a CHOP Channel Struct, based on a given Channel name.
- Get Num Channels: Return the number of CHOP Channels given a CHOP Struct. You can get to the same result using the length node after breaking a CHOP Struct and using the Channel array pin.
- Get Num Samples: Return the number of Samples given a CHOP Struct. You can get to the same result using the length node after breaking a Channel CHOP Struct and using the Samples (values) array pin.
- CHOP Clear: Remove all channels and samples on a given CHOP
- Is CHOP Input Valid: Verify if the CHOP is valid. A CHOP is valid when all channels have the same number of samples. A CHOP with no channels or with only empty channels is considered valid. A CHOP retruned from Get TouchEngine Output is always valid.
### TOPs
- Keep Frame Texture: Keeps the frame texture retrieved from Get TouchEngine Output. When retrieving a TOP, Get TouchEngine Output returns a temporary texture that will go back into a texture pool after another value has been retrieved from TouchEngine, for performance. If the texture needs to be kept alive for longer, this function needs to be called to ensure the frame texture is removed from the pool and will not be overriden.
- Refresh Texture Sampler: Force the recreation of the internal Texture Samplers based on the current value of the Texture Filter, AddressX, AddressY, AddressZ, and MipBias. This can be called on any type of textures (even the ones not created by TouchEngine), but it might not work on all types if they have specific implementations. Returns true if the operation was successful (the Texture and its resource were valid)
### DATs
- Get Cell: returns FString of value at index row, col
- Get Cell by Name: same as get cell, but uses the first cell of the row and column as their names
- Get Row: returns a FString array of the row at index row
- Get Row by Name: same as get row, but uses the first cell of the row as its name
- Get Column: returns a FString array of the column at index col
- Get Column by Name: same as get column, but uses the first cell of the column as its name
### Debug
- CHOP To Debug String: Helper function that converts a CHOP to a string. Useful for debugging. Shouldn't be used outside of the debugging context.
- CHOP Channel To Debug String: Helper function that converts a CHOP to a string. Useful for debugging. Shouldn't be used outside of the debugging context.
