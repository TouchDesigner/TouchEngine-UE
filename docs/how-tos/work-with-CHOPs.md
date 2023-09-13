# How to work with CHOPs

As of version 1.2.0, many new features and workflows are available around CHOPs. We recommend users visit our Sample project and have a look at the few CHOP related samples, which were all updated.

## CHOPs in the details panel

When CHOPs inputs or outputs are present in the .tox being loaded in the TouchEngine, you can select the TouchEngine Actor in the world outliner and look in the details panel.

The details panel Component Settings section will display useful information for you to work with CHOPs such as their names, number of channels and samples being received… etc.

![../assets/how-tos-CHOPs/chop_input_channels_data.png?raw=true](../assets/how-tos-CHOPs/chop_input_channels_data.png?raw=true)

## Set channel names

You can set channel names by creating explicitly a CHOP Channel using the Make node.

![../assets/how-tos-CHOPs/chops_channel_named.png?raw=true](../assets/how-tos-CHOPs/chops_channel_named.png?raw=true)

Just pass a value in the Name property.

If no name is specified, it will follow the TouchDesigner default naming convention.

## Merge channels together

You can easily prepare your CHOP to receive multiple channels by using the basic array nodes of Unreal.

![../assets/how-tos-CHOPs/chop_input_channels_merge.png?raw=true](../assets/how-tos-CHOPs/chop_input_channels_merge.png?raw=true)

You first create your CHOP Channels, put them together in an array, and make it a CHOP.

## Pass various types directly to Set TouchEngine Input

The Set TouchEngine Input node remains flexible. You can pass arrays of float and this will internally make a valid CHOP internally. Note that this specific case will create a single Channel of many samples.

Additionally, you can pass a Channel directly to the Set TouchEngine Input, or an array of channels, and they will be turned to valid CHOPs internally.

## Get Channel, Get Channel by Name

> ⚠️ **Note** ⚠️ that working with Get Channel, Get Num Samples **comes at a performance cost**. Those methods are helper that can be used on small datasets (read, small number of samples / channels). Users should prefer working with traditional getters, by ref, on larger sets.

Those are you base nodes when working with CHOPs after a Get TouchEngine Output node.

They will return a CHOP struct that you can break into channels and / or samples.

## Is Valid

This Is Valid node is slightly different than the regular Is Valid node. A CHOP is considered valid even if they are no CHOP Channels or samples. Therefore, as long as the object itself is valid and considered a CHOP, this Is Valid node should return True.

Directly placed after a Get TouchEngine Output node, the Is Valid node is not needed, since you can use the Result boolean to confirm if the output is valid or not.

## Helpers

Helpers method are available for debugging purpose. CHOP To String and CHOP Channel To String can be used to quickly get the values to a string to be logged / printed out.

Note that those are coming at a performance cost and shouldn't be abused, and stay in the debugging realm.

## Optimize

Based on the context, they are many ways you can optimize your code with those new data types. You can read and write more freely, loop through channels, work on a given specific channel, name specific channels... etc.

> ⚠️ UStructs are a bit more complex but complete objects. What it means for CHOPs is that when passing objects from one blueprint to the other, make sure you pass them by reference, unless a copy is justified. There is a cost coming with creating copies, especially when called within a loop. See bellow, how we pass the reference to a CHOP from one BP to another BP method.

![../assets/how-tos-CHOPs/chops_to_other_bp_method.png?raw=true](../assets/how-tos-CHOPs/chops_to_other_bp_method.png?raw=true)
![../assets/how-tos-CHOPs/chops_reference_in_method.png?raw=true](../assets/how-tos-CHOPs/chops_reference_in_method.png?raw=true)

## Working with break / make, split pins.

![../assets/how-tos-CHOPs/chop_splitting.png?raw=true](../assets/how-tos-CHOPs/chop_splitting.png?raw=true)

The following approaches are giving the same result.

You don't necessarily have to use nodes when working with UStructs. You can work on pins directly and use the worflows that you prefer or that seem fit for your specific cases.

You can also split / recombine pins on CHOP Channels.

> ⚠️ **Note** ⚠️ that working with Get Channel, Get Num Samples **comes at a performance cost**. Those methods are helper that can be used on small datasets (read, small number of samples / channels). Users should prefer working with traditional getters, by ref, on larger sets.
