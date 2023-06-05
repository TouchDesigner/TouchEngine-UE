# Main features

## Details Panel
![assets/main-features/touchengine_component_panel.png?raw=true](assets/main-features/touchengine_component_panel.png?raw=true)

<!-- TODO: Explain each bit of the details panel -->

The details panel is your go-to for the basic setup of the TouchEngine Component.

* Tox Asset: A property that can receive a ToxAsset, a .uasset pointing to a .tox file.
* Reload Tox: Forces the .tox to reload in the TouchEngine instance. This is useful if you changed parameters, inputs... etc.
* Allow running in editor: When Allow Running in Editor is toggled on, the TouchEngine is loaded with the .tox and can be used without the project running as packaged or in PIE mode. In blueprint, developers should make use of the TouchEngine Component Begin Play and End Play events. See [How To: Work In Editor 🔗](how-tos/work-in-editor.md).
* Load on begin play: In PIE or packaged mode, the TouchEngine will start and load the .tox as soon as the play event is called.
* Cook Mode: See [Sync Modes 🔗](sync-modes.md)
* Send Mode: Every Frame sets and gets outputs every frame, while on access only sends them when the variable types are accessed via the blueprint nodes.
* TE Frame Rate: The frame rate at which the TouchEngine subprocess should be running.
<!-- * Time Scale: TODO: To complete after confirming -->
* Component Settings: This is the section where all the Unreal properties specific to the currently loaded .tox are dynamically created. They are sorted in subsections, Parameters, Inputs and Outputs. Parameters are accessed with the nodes Get / Set TouchEngine Parameters, while Inputs and Outputs are accessed using Set TouchEngine Input and Get TouchEngine Output nodes respectively.

## Events
![assets/main-features/touchengine_component_events.png?raw=true](assets/main-features/touchengine_component_events.png?raw=true)

- On Tox Loaded
    - Called when the TouchEngine instance loads the .tox file. 
- On Tox Reset
    - Called when the TouchEngine instance is reset, and data is cleared.
- On Tox Failed Load
    - Called when the TouchEngine instance failed to load the .tox file.
- On Tox Unloaded
    - Called when the TouchEngine instance unloads the .tox file.
    - It might be called multiple multiple times and it is also called after On Tox Failed Load.
- On Set Inputs
    - Called before sending the inputs to the TouchEngine.
- On Outputs Received
    - Called after receiving the outputs from the TouchEngine.
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
- On Component Deactivated
    - Called when a Component has been deactivated.

## Nodes
![assets/main-features/touchengine_nodes01.png?raw=true](assets/main-features/touchengine_nodes01.png?raw=true)
![assets/main-features/touchengine_nodes02.png?raw=true](assets/main-features/touchengine_nodes02.png?raw=true)
<!-- TODO: Update pictures with new screenshots when menu is reorganized -->


### Interacting with component
#### States
- Start TouchEngine: Start and run the TouchEngine instance, will call ToxLoaded or ToxFailedLoad callbacks depending on the result.
- Can Start: Should UI, players, or other means, be allowed to start the TouchEngine.
- Is Running: Is the TouchEngine currently running.
- Load Tox: Load or reload the currently loaded .tox file.
- Stop TouchEngine: Stops and delete the TouchEngine instance.
#### Properties
- Get TouchEngine Input: Get the current object set for a TouchEngine Input.
- Set TouchEngine Input: Set the object to be passed from Unreal to the TouchEngine, to a given Input. Must be of a valid format for CHOP, DAT or TOP.
- Get TouchEngine Output: Get the object sent out of the TouchEngine and received by Unreal. Can be a CHOP Struct, a DAT Object, or a texture2D (TOP).
- Set TouchEngine Parameter: Set the value to be passed to a TouchEngine parameter at the blueprint level. Note that a parameter changed through the details panel will be sent to the TouchEngine as well.
- Get TouchEngine Parameter: Get the current value set for a TouchEngine Parameter.

- Get TouchEngine Component: Returns the TouchEngine Component assigned to a TouchEngine Actor.

### CHOPs
- Get Channel: Return a CHOP Channel Struct, based on a given Channel index.
- Get Channel by Name: Return a CHOP Channel Struct, based on a given Channel name.
- Get Num Channels: Return the number of CHOP Channels given a CHOP Struct. You can get to the same result using the length node after breaking a CHOP Struct and using the Channel array pin.
- Get Num Samples: Return the number of Samples given a CHOP Struct. You can get to the same result using the length node after breaking a Channel CHOP Struct and using the Samples (values) array pin.
- Clear: Remove all channels and samples on a given CHOP
- Is Valid: Verify if the CHOP is valid. A CHOP is valid when all channels have the same number of samples. A CHOP with no channels or with only empty channels is considered valid.
- CHOP To String: Helper function that converts a CHOP to a string. Useful for debugging. Shouldn't be used outside of the debugging context.
- CHOP Channel To String: Helper function that converts a CHOP to a string. Useful for debugging. Shouldn't be used outside of the debugging context.

### TOPs
- There is no TOPs specific methods at the moment. TouchEngine TOPs are treated as UE Texture2D values.

### DATs
- Get Cell: returns FString of value at index row, col
- Get Cell by Name: same as get cell, but uses the first cell of the row and column as their names
- Get Row: returns a FString array of the row at index row
- Get Row by Name: same as get row, but uses the first cell of the row as its name
- Get Column: returns a FString array of the column at index col
- Get Column by Name: same as get column, but uses the first cell of the column as its name
