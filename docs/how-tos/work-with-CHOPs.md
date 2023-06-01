# How to work with CHOPs

<!-- TODO: Document the following points

- *Can now set specific named channels.*
- *Can merge channels together.*
- *Can quickly pass from CHOP Channel to String or CHOP Channel to array.*
- *More freedom on developments patterns. Can optimize based on context.* -->

# CHOPs in the details panel

When CHOPs inputs or outputs are present in the .tox being loaded in the TouchEngine, you can select the TouchEngine Actor in the world outliner and look in the details panel.

The details panel Component Settings section will display useful information for you to work with CHOPs such as their names, number of channels and samples being received… etc.

![../assets/how-tos-CHOPs/chop_input_channels_data.png?raw=true](../assets/how-tos-CHOPs/chop_input_channels_data.png?raw=true)

# Set channel names

You can set channel names by creating explicitly a CHOP Channel using the Make node.

![../assets/how-tos-CHOPs/chops_channel_named.png?raw=true](../assets/how-tos-CHOPs/chops_channel_named.png?raw=true)

Just pass a value in the Name property.

If no name is specified, it will follow the TouchDesigner default naming convention.