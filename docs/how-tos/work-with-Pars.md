# How to work with parameters

To work with parameters, simply interact with the details panel.

Any property changed that is in the section Component Settings -> Parameters in PIE or in Editor mode (when the TouchEngine is running) will update the parameter internally.

You can, from blueprints, use the Set / Get TouchEngine Parameter nodes.

Not all parameters types are compatible and not all of them are useful in this specific TouchEngine / Unreal context.

The compatible types are:
* File
* Float (of different sizes)
* Folder
* Int (of different sizes)
* Menu
* Pulse
* Rgba
* Rgb
* Str
* Strmenu
* Toggle
* Uv
* Uvw
* Wh
* Xy
* Xyz
* Xyzw

The main idea here is working with floats, ints, or strings.

Any parameter that is of size > 1 is compatible with arrays of floats, or in some cases vectors.

Menus are working as indices, with integers.

# Slider type properties (ints, floats)

When updating a value using the slider, the change doesn’t trigger the construction blueprint of the actor to re-init anymore. This prevent frame drops when tweaking a property. However, this means that the value will update and be passed to the TouchEngine Component only when the mouse is released.

> ⚠️TouchEngine supports range min/max and clamp min/max values. Users should set those to avoid cases where incredibly small or incredibly high values are being passed from / to TouchEngine / Unreal.
