# How to use the plugin in editor mode

# Start / stop

When in editor mode, select any TouchEngine Actor or Actor with a TouchEngine Component re-parented.

Toggle on the “Allow Running in Editor” button.

![../assets/how-tos-editor-mode/in_editor_mode_toggle.png?raw=true](../assets/how-tos-editor-mode/in_editor_mode_toggle.png?raw=true)

After 1-10 seconds, depending on your hardware, the component should be loaded in a TouchEngine process and you should be able to interact with any of its parameters or inputs / outputs.

While in the blueprint editor, editing the blueprint and saving will trigger the blueprint to recompile and the component to be unloaded / reloaded into the engine.

# Begin Play / End Play events

Use the TouchEngine events when needed in your blueprints. On TouchEngine Components, they are custom versions of the vanilla Begin Play and End Play events that are designed to be triggered when the `Allow Running in Editor` is turned on or off.

# Breakpoints and debugging

Blueprints Breakpoints do not work with `Allow Running in Editor` turned on as the components would be running from the same thread as the Editor itself.

Though, you should always see the data flow on a running component (similar to the screenshot bellow) and also see messages in the output log that the breakpoints are actually triggered.

![../assets/how-tos-editor-mode/in_editor_mode_debug.png?raw=true](../assets/how-tos-editor-mode/in_editor_mode_debug.png?raw=true)