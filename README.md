# TouchEngine-UE Plugin Repository

**Current Unreal Engine version:** `5.0.2`

**Current TouchEngine version:** `2021.16410` (Latest official, [download link](https://derivative.ca/download))

The TouchEngine Plugin currently supports Custom Parameters, CHOP input/output, TOP input/output and DAT input/output.

## TouchEngine Plugin Setup Guide

### Installation

Go to the directory with the .uproject for the project you would like to install the plugin for.
If your project does not already have a `Plugins` folder, create a folder called `Plugins` within that directory.
Place the `TouchEngine-UE` folder into the Plugins folder.

After you’ve followed these steps, to be sure that you’ve installed the plugin correctly, do the following:
- Open the project in Unreal Engine
- Under Edit -> Plugins, select the “Project” category from the list on the left. 

This should show `TouchEngine` along with any other custom plugins you’ve installed. Make sure the `Enabled` check box is checked, and you’ve successfully installed the plugin.

Note: You should now restart Unreal Engine for the plugin changes to take effect.

### Obtaining a TOX file

The file that you’ll need to be able to implement a project designed in TouchDesigner into an Unreal Engine project is a .tox file. To generate one of these from a TouchDesigner project, first open the project in TouchDesigner.

Navigate to the network containing the component you want to turn into a .tox file. Right-click on the component and select “Save Component .tox…”. Save the file inside the UE project’s “Content” folder.

![TOX](ReadmePictures/im1.png?raw=true "Obtaining Tox File")

### Adding the “TouchEngine Component” to an Actor

Currently, the component can only be added directly to C++ classes. In order to get the component on a Blueprint class, you'll need to derive that class from a C++ class with the component set up in it. 

If you're creating a new Actor object, parent it to a C++ class with a TouchEngine Compoment added. 

![image](https://user-images.githubusercontent.com/29811612/136610156-8207723b-43c7-42cb-bd94-743517db8932.png)

For adding the component to an existing Actor, you're going to need to either add a TouchEngine Component into the base C++ class or reparent the Blueprint. To do this, open the Blueprint you want to add the component to, select "File" at the top, and select "Reparent Blueprint". 

![image](https://user-images.githubusercontent.com/29811612/136609219-a10d15be-613c-4b14-935d-26eb34466df0.png)

If you don't have a C++ class with a TouchEngine Component, select "TouchEngineActor" from the dialog that pops up. This is a default Actor class with the TouchEngine Compoment attached. 

![image](https://user-images.githubusercontent.com/29811612/136609519-a892963b-d77d-408a-b5eb-768db4bd0ceb.png)

Once you do this, you should see the TouchEngine Component appear in the Blueprint's "Components" menu. 

![image](https://user-images.githubusercontent.com/29811612/136609795-7531b88b-c14f-4545-a7b4-91f07fc9b45b.png)

![DETAILS_PANEL](ReadmePictures/im3.PNG?raw=true "Details Panel")

### Adding events from the TouchEngine Component to your Blueprint

When in the blueprint editor with a TouchEngine Component, with the TouchEngine Component selected (Components pane, on the left), you will see in Details panel on the right in the list of events various events that can be used in the current blueprint and related to the TouchEngine Component.

![EVENTS](ReadmePictures/TouchEngine_Component_Events.PNG?raw=true "TouchEngine Component Events")

### Accessing Input and Output Variables
The “Variables” list in the details panel underneath the .tox file path entry box will contain all input and output variables of the currently loaded .tox file. Input variables will be modifiable in the panel, but output variables will not be. Hovering over the display name for each variable will show the variable identifier. It is the path needed to get / set the variable while in the blueprint editor and pass (or get) changes to (or from) the TouchEngine Component.

There are two ways to set input values with the plugin but only one way to get outputs. 

The first way to set input is to modify the value in the details panel, which will set the variable’s default value. The second way to set inputs as well as the way to get outputs requires the use of blueprints. Select the “Event Graph” tab and right click anywhere in the window to bring up the list of available blueprint nodes. Typing in “TouchEngine” should show the two blueprint nodes “Set TouchEngine Input” and “Get TouchEngine Output”.

![NODES](ReadmePictures/im4.PNG?raw=true "Blueprint nodes")

For inputs, first enter the variable’s identifier (found by hovering over the variable name in the details panel in the “Input Name” entry box, note that these correspond to the Parameter Names in the TouchDesigner component). Second, drag the “TouchEngineComponent” object out from the components list on the left and plug it into the “Touch Engine Component” pin in the blueprint node. Last, drag the input value of the variable you want to set into the “Value” pin. If the pin does not connect, that means the type is not an acceptable input value. Below are two examples of input nodes set up correctly.

![NODES_SETUP](ReadmePictures/im5.PNG?raw=true "Using the nodes")

The result value will be true if the input was successfully set, false if it was not. 

Output values are much the same as input values, but with the “Get TouchEngine Output” blueprint node. First, enter the variable’s identifier (found by hovering over the variable name in the details panel in the “Output Name” entry box). Second, drag the “TouchEngineComponent” object out from the components list on the left and plug it into the “Touch Engine Component” pin in the blueprint node. Last, drag the output value pin into where you would like to use the value. If the pin does not connect, that means the type is not an acceptable output value. Below are two examples of output nodes set up correctly.

![NODES_FINISH](ReadmePictures/im6.PNG?raw=true "Finished setup")


### Sequencing

For setting up TouchEngine variables with the Level Sequencer, add a variable corresponding to the TouchEngine variable to the Blueprint holding it, and set it with either "Get TouchEngine Input Latest Value" if the variable is an input or "Get TouchEngine Output" if it's an output variable. 

Make sure the variable added to the Blueprint has the values "Exposed to Cinematics" and "Instance Editiable" checked to be available in the Level Sequencer.

![image](https://user-images.githubusercontent.com/29811612/140799698-0a87d958-0a64-460a-b843-39351cf146f0.png)


## Main features

![image](https://user-images.githubusercontent.com/29811612/122442635-c72d8980-cf6c-11eb-8a53-c35ef09ba5b8.png)

### General

- Set TouchEngine Input: sets the value of an input parameter
- Get TouchEngine Input Current Value: gets the current value of an input parameter
- Get TouchEngine Output: gets the value of an output parameter
- IsRunning: returns whether or not the instance is currently running. 
- Start TouchEngine: creates the TE Instance, will call ToxLoaded or ToxFailedLoad callbacks depending on the result
- Stop TouchEngine: destroys the TE Instance
- ReloadTox: does nothing at runtime, reloads stored parameters for UI use

### TOPs

There is no TOPs specific methods at the moment. TouchEngine TOPs are treated as UE Texture2D values.

### CHOPs

- Get Channel: returns a float array of the channel at channel index
- Get Channel by Name: same as get channel, but gets the value by name instead of by index
- Clear: empties the CHOP struct

### DATs

- Get Cell: returns FString of value at index row, col
- Get Row: returns a FString array of the row at index row
- Get Column: returns a FString array of the column at index col
- Get Cell by Name: same as get cell, but uses the first cell of the row and column as their names
- Get Row by Name: same as get row, but uses the first cell of the row as its name
- Get Column by Name: same as get column, but uses the first cell of the column as its name

## Known issues and current limitations

- TouchEngine only works with DX11 renderer so far. Although DX12 is partially implemented as well.

## TouchEngine-UE Plugin Samples documentation

This repository is covering the TouchEngine-UE Plugin, for samples and samples documentation of the TouchEngine-UE Plugin, follow [this link](https://github.com/TouchDesigner/TouchEngine-UE-Samples/).

## C++ Documentation

### ATouchEngineActor

Derive from this actor class to add a TouchEngine instance to an object. This class holds no additional functionality other than this, but is required to get the details panel to work correctly. Alternatively add the TouchEngineComponent class to another C++ AActor subclass for the same effect, but adding the component directly to a blueprint will cause issues. 

### UTouchEngineComponent

The TouchEngine Component is the ActorComponent that adds TouchEngine functionality to an actor when added to it. Adding this directly to a blueprint actor will break some of the details panel functionality, but will work as expected when added to a C++ actor.

#### Blueprint Accessable Variables

- ToxFilePath: The path to the .tox file to load
- CookMode: Mode for component to run in. Modes are: 
  - Synchronized: Starts the cook at the beginning of the frame, stalls the engine on tick until the cook is complete
  - DelayedSynchronized: Starts the cook on tick, stalls the engine on the next tick if the cook isn't finished
  - Independent: Starts the cook and gets the output when the cook is ready. Does not stall the engine
- SendMode: Mode for the component to set and get variables. Modes are:
  - EveryFrame: Sends inputs or gets outputs from the TouchEngine every frame
  - OnAccess: Only sends inputs or gets outputs when the variable is accessed via blueprints or the details panel
- TEFrameRate: Frame rate for the TouchEngine instance to run at
- LoadOnBeginPlay: If true, starts the engine on begin play of the component. If false, StartTouchEngine must be called manually.

#### Blueprint Accessable Functions

- ReloadTox: Reloads the currently loaded tox files
- StartTouchEngine: Creates and starts the TouchEngine instance
- StopTouchEngine: Stops and destroys the TouchEngine instance
- IsRunning: Returns whether or not the TouchEngine instance is currently running

#### Blueprint Accessable Delegates

- OnToxLoaded: Called when the TouchEngine instance loads a tox file
- OnToxFailedLoad: Called when the TouchEngine instance fails to load a tox file
- SetInputs: Called before sending inputs to the TouchEngine instance based on cook mode
- GetOutputs: Called before getting outputs from the TouchEngine instance based on cook mode

### FTouchEngineDynamicVariableContainer

Holds an input and output array of TouchEngine variables as well as some functions to interface with the TouchEngine instance.

### FTouchEngineDynamicVariable

Holds a void pointer that contains the value of the variable as well as information about what type the variable is. It also contains the variable name, label, and identifier as specified by the TouchEngine instance.

### UTouchEngineInfo

Wrapper for UTouchEngine class that makes accessing the TouchEngine instance more straightforward and includes error checking. Contains utility funtions for getting information about the TE instance as well as getting / setting parameter values.

### UTouchEngine 

Contains the TouchEngine instance as well as DirectX11 objects and other low level functionality. Interfaces with the TEInstance to get parameter information and create UE textures from DX11 textures.

### UTouchBlueprintFunctionLibrary

Contains functions to get / set variable values in the TouchEngineComponent that are wrapped by the TouchEngineInput, TouchEngineGetInput, and TouchEngineGetOutput K2 blueprint nodes. 

### FTouchEngineIntVector4

Int vector 4 class that we expose to blueprints since UE's base int vector class is not exposed to blueprints.

### UTouchEngineSubsystem

Loads the TouchEngine libraries and dlls. Also holds parameter information used to display values in the editor details panels without having to keep a TouchEngine instance running at all times. 

### UTouchInputK2Node, UTouchInputGetK2Node, and UTouchOutputK2Node

K2 nodes that handle the "Set TouchEngine Input", "Get TouchEngine Input Latest Value", and "Get TouchEngine Output" blueprint nodes respectfully.

### FTouchEngineDynVarDetsCust

Details panel customization for the FTouchEngineDynamicVariableContainer struct. This handles drawing the editable values both in the blueprint details panel and in the world details panel for the TouchEngine dynamic variable container in its TouchEngineComponent.
