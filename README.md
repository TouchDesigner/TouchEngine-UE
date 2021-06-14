# TouchEngine-UE4 Plugin Repository

**Current Unreal Engine version:** `4.26.2`

**Current TouchEngine version:** `2021.14010` ([download link](https://www.dropbox.com/s/s51094yygmzm5du/TouchDesigner.2021.14010.exe?dl=0))

The TouchEngine Plugin currently supports Custom Parameters, CHOP input/output, TOP input/output and DAT input/output.

## TouchEngine Plugin Setup Guide

### Installation

Go to the directory with the .uproject for the project you would like to install the plugin for.
If your project does not already have a `Plugins` folder, create a folder called `Plugins` within that directory.
Place the `TouchEngine-UE4` folder into the Plugins folder.

After you’ve followed these steps, to be sure that you’ve installed the plugin correctly, do the following:
- Open the project in Unreal Engine
- Under Edit -> Plugins, select the “Project” category from the list on the left. 

This should show `TouchEngine` along with any other custom plugins you’ve installed. Make sure the `Enabled` check box is checked, and you’ve successfully installed the plugin.

### Obtaining a TOX file

The file that you’ll need to be able to implement a project designed in TouchDesigner into an Unreal Engine project is a .tox file. To generate one of these from a TouchDesigner project, first open the project in TouchDesigner.

Navigate to the network containing the component  you want to turn into a .tox file. Right-click on the component and select “Save Component .tox…”. Save the file inside the UE4 project’s “Content” folder.

![TOX](ReadmePictures/im1.png?raw=true "Obtaining Tox File")

### Adding the “TouchEngine Component” to an Object

Step one is to create an object that you want to attach a TouchEngine component to, and open its blueprint. The components list is in the top left corner of the blueprint window, with a green “Add Component” button. Type in “TouchEngine” and the TouchEngine Component should show up. If it does not appear here, make sure all the steps in Installation correctly. 

![COMPONENT](ReadmePictures/im2.png?raw=true "Adding Component")

Click on the “TouchEngine Component” option and it should add one to the object. Clicking on the new component will reveal its options on the right side of the window, where under the category “Tox File” there will be a box to specify the .tox file path and a list of the variables in that file once loaded. The path you enter in the “Tox File Path” box is relative to the project’s content folder.

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

## Main features

![image](https://user-images.githubusercontent.com/10253645/121780812-fbb9d380-cb6f-11eb-9bc3-9114c32fd046.png)

### General

- Set TouchEngine Input: sets the value of an input parameter
- Get TouchEngine Input Current Value: gets the current value of an input parameter
- Get TouchEngine Output: gets the value of an output parameter
- IsRunning: returns whether or not the instance is currently running. 
- Start TouchEngine: creates the TE Instance, will call ToxLoaded or ToxFailedLoad callbacks depending on the result
- Stop TouchEngine: destroys the TE Instance
- ReloadTox: does nothing at runtime, reloads stored parameters for UI use

### TOPs

There is no TOPs specific methods at the moment. TouchEngine TOPs are treated as UE4 texture2D values.

### CHOPs

- Get Channel: returns a float array of the channel at channel index
- Get Channel by Name: same as get channel, but gets the value by name instead of by index
- Clear: empties the CHOP struct

### DATs

- Get Cell: returns FString of value at index row, col

## Known issues and current limitations

- TouchEngine only works with DX11 renderer so far. Although DX12 is partially implemented as well.
- The plugin was not tested against TouchEngine in TouchDesigner's Vulkan build

## TouchEngine-UE4 Plugin Samples documentation

This repository is covering the TouchEngine-UE4 Plugin, for samples and samples documentation of the TouchEngine-UE4 Plugin, follow [this link](https://github.com/TouchDesigner/TouchEngine-UE4-Samples/).
