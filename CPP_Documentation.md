# UE C++ Workflow Documentation

## The Dynamic Variables
The first step to understanding how this plugin functions is to look at the file "TouchEngineDynamicVariableStruct.h". This file contains the structs "FTouchEngineDynamicVariableContainer" and "FTouchEngineDynaimcVariable".

The Dynamic Variable Container contains an input and output array of Dynamic Variables and functionality to help with the getting / setting of those variables. This struct MUST be placed in a UTouchEngineComponent, as it holds a pointer to that class ("parent") that is referenced often (especially in the details panel customization) in order to gain access to the TouchEngine instance.

The Dynamic Variable contains a void pointer as well as information about what type the value held in that void pointer is supposed to be. This is done so that every variable type coming from the TouchEngine instance can be stored in an array and displayed as easilly as possible to the details panel. 

The memory of each dynamic variable is managed by the "SetValue" functions, which delete and allocate memory based on the type of variable specified by VarType and IsArray. The Clear function will also allow for manual deletion of the memory. 

To get the data to save to the disk, a custom serializer is used that switches on the type of the variable and writes / reads the data as the correct value.

The FTouchEngineDynamicVariableContainer struct is customized (displayed to the details panel via slate) by the class "FTouchEngineDynamicVariableDetailsCustomization" in the TouchEngineEditor module (in the file "TouchEngineDynVarDetsCust" due to file length limitations). 

The "CustomizeHeader" function queries the engine instance to see if the ToxFile has been loaded. It creates a header and then for the value draws a throbber if the engine is loading, writes error messages if it has finished loading but has failed, or does nothing if the tox file has been loaded successfully. 

The "CustomizeChildren" function draws all the loaded data in the Dynamic Variable Container by switching on the type and drawing the corresponding widget and binding delegates to it. The Details Panel Customization receives the callbacks and passes them to the corresponding dynamic variable because after changing a value in the details panel, it is possible that the dynamic variable will be destroyed before the callback is called. Instead, we store the identifier of the variable being modified in the callback and attempt to grab the variable from the stored Dynamic Variable Container within the Details Panel Customization to ensure we get the valid current Dynamic Variable. 

More complicated types to draw the widgets for (Textures, Vectors, etc) are simplified by adding an editor only property to the dynamic variable that will be modified when the widget in the details panel is modified before the callback comes through to the Details Panel Customization. These types only exist for editor purposes and should never be accessed in non editor code or without a #WITH_EDITOR check.

When accessing the stored Dynamic Variables or broadcasting callbacks from the DynamicVariableContainer into the Details Panel, it's very possible that the Details Panel has been deleted and recreated since they were bound for the first time. The Details Panel and the Dynamic Variables exist on different threads, and both are often automatically deleted and recreated by the engine during actions such as changing a value in the details panel, moving an object in the world, or other very common actions. Ensuring all accesses to these variables are checked and all delegates are properly bound and unbound is very important to prevent crashing.

## UTouchEngineComponent
The UTouchEngineComponent class handles creating the UTouchEngineInfo object as well as the FTouchEngineDynamicVariableContainer class. It holds blueprint functionality for interfacing with the TouchEngine instance as well as variables to modify the functionality.

## UTouchEngineInfo
The UTouchEngineInfo class mostly handles interfacing with the UTouchEngine class to make the calls cleaner. It can be treated as the link between the UTouchEngineComponent and the UTouchEngine class.

## UTouchEngine
The UTouchEngine class interfaces with the TouchEngine library and DLL to send calls to the API. It wraps the TouchEngine instance.

## UTouchEngineSubsystem
The UTouchEngineSubsystem stores parameter data for loaded tox files. It helps decrease the number of TouchEngine instances running in editor by remembering what parameters have been loaded for each tox file and only creating a new TouchEngine instance when the "Reload Tox" function is called or when a new tox file is loaded. It is only accessed by the UTouchEngineComponent in editor, as when the game is running it is necessary to have a TouchEngine instance running for each object. 

## Touch Blueprint Function Library
Inside the TouchBlueprintFunctionLibrary.h file is functions to help with setting / getting typed inputs / outputs from the Dynamic Variables. These functions are wrapped by the three K2 nodes TouchInputGetK2Node, TouchInputK2Node, and TouchOutputK2Node. These nodes have a wildcard value that will be cast to the correct value type if the pin type matches one of the valid types (checked in the CheckPinCategory function of each K2 node) and then passed to the correct function as found by the "FindInputGetterByType", "FindInputByType", or "FindOutputByType" functions.

## Debugging tips: 
For blueprint functionality, the three Input / Output nodes (Set TouchEngine Input, Get TouchEngine Output, Get TouchEngine Input Latest Value) are all wrapped by three functions in the TouchBlueprintFunctionLibrary.cpp file - FindSetterByType, FindGetterByType, and FindInputGetterByType. These functions are called from the corresponding K2Node classes (TouchInputK2Node, TouchOutputK2Node, TouchInputGetK2Node) in their ExpandNode functions, which pass the pin type and struct name to the wrapper functions.

In the wrapper functions, we switch on the incoming type to determine the function to pass back to the K2Node to bind that c++ function to the blueprint node to be called at runtime. This process happens on compile, and so these three wrapper functions will be called when hitting the compile button in a blueprint that contains one of these three nodes, so if there are unexpected errors coming from these blueprint nodes placing a breakpoint in the corresponding wrapper function and hitting compile is the first step I would use to debug them. From there, you can see which function is being bound in the blueprint node and you can place a breakpoing there to hit at runtime. 

For errors with the details panel, the two main functions to look at are the CustomizeHeader and CustomizeChildren functions in the TouchEngineDynVarDetsCust.cpp file. The CustomizeHeader is the smaller of the two functions and only controlls the first line of the customization of the TouchEngineDynamicVariableContainer, so only the "Tox Parameters" header, the throbber if the tox file is still loading, and the error message displayed if the tox file has failed to load.

The CustomizeChildren function is what handles each individual variable within the TouchEngineDynamivVarableContainer, so if there are issues with certain types of variables or with displaying their values then this is the place to look. For both of the arrays (Input and Output) we switch on each variable type and display the variable identifier and bind values to when those variables are changed. Putting a breakpoing in the case section of each variable type you're seeing issues with is a good first step for debugging.

Another thing to keep in mind when working with the details panel is that is deconstructed and reconstructed quite frequently (such as when moving the object the dpc is attached to, changing a variable type, or clicking off the object). If you're getting crashes when broadcasting callbacks bound to dpc functions, the callbacks may not have been unbound / rebound correctly.
