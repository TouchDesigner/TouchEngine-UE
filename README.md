# TouchEngine-UE4

TouchEngine Plugin Setup Guide


Installation

Go to the directory with the .uproject for the project you would like to install the plugin for.
If your project does not already have a “Plugins” folder, create a folder called “Plugins” within that directory.
Place the “TouchEngine-UE4” folder into the Plugins folder.

After you’ve followed these steps, to be sure that you’ve installed the plugin correctly, do the following:
Open the project in Unreal Engine
Under Edit -> Plugins, select the “Project” category from the list on the left. This should show “TouchEngine” along with any other custom plugins you’ve installed. Make sure the “Enabled” check box is checked, and you’ve successfully installed the plugin.


Obtaining a TOX file

The file that you’ll need to be able to implement a project designed in TouchDesigner into an Unreal Engine 4 project is a TOX file. To generate one of these from a TouchDesigner project, first open the project in TouchDesigner.

Scroll out to the top most layer of the project and right click on the component that you want to turn into a TOX file. Click on the “Save Component .tox…” option, and you will be prompted to save the file. Save it inside the UE4 project’s “Content” folder.



Adding the “TouchEngine Component” to an Object

Step one is to create an object that you want to attach a touch engine component to, and open its blueprint. The components list is in the top left corner of the blueprint window, with a green “Add Component” button. Type in “TouchEngine” and the TouchEngine Component should show up. If it does not appear here, make sure all the steps in Installation correctly. 



Click on the “TouchEngine Component” option and it should add one to the object. Clicking on the new component will reveal its options on the right side of the window, where under the category “Tox File” there will be a box to enter the tox file path in and a list of the variables in that file once loaded. The path you enter in the “Tox File Path” box is relative to the project’s content folder.



Accessing Input and Output Variables
The “Variables” list in the details panel underneath the tox file path entry box will contain all input and output variables of the currently loaded tox file. Input variables will be modifiable in the panel, but output variables will not be. Hovering over the display name for each variable will show the variable identifier, which is the path needed to get / set the variable in TouchEngine.

There are two ways to set input values with the plugin but only one way to get outputs. 

The first way to set input is to modify the value in the details panel, which will set the variable’s default value. The second way to set inputs as well as the way to get outputs require a bit of blueprinting. Select the “Event Graph” tab and right click anywhere in the window to bring up the list of available blueprint nodes. Typing in “TouchEngine” should show the two blueprint nodes “Set TouchEngine Input” and “Get TouchEngine Output”. 



For inputs, first enter the variable’s identifier (found by hovering over the variable name in the details panel in the “Input Name” entry box. Second, drag the “TouchEngineComponent” object out from the components list on the left and plug it into the “Touch Engine Component” pin in the blueprint node. Last, drag the input value of the variable you want to set into the “Value” pin. If the pin does not connect, that means the type is not an acceptable input value. Below are two examples of input nodes set up correctly.

The result value will be true if the input was successfully set, false if it was not. 

Output values are much the same as input values, but with the “Get TouchEngine Output” blueprint node. First, enter the variable’s identifier (found by hovering over the variable name in the details panel in the “Output Name” entry box. Second, drag the “TouchEngineComponent” object out from the components list on the left and plug it into the “Touch Engine Component” pin in the blueprint node. Last, drag the output value pin into where you would like to use the value. If the pin does not connect, that means the type is not an acceptable output value. Below are two examples of output nodes set up correctly.

