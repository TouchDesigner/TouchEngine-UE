# Installation

Go to the Unreal Engine project directory for which you want to install the plugin. You should see at the root a `.uproject` file. If your project does not already have a `Plugins` folder, create a folder called `Plugins` within that directory. Place the `TouchEngine` folder into the Plugins folder.

After you’ve followed these steps, to be sure that you’ve installed the plugin correctly, do the following:

- Open the project in Unreal Engine
- Under Edit -> Plugins, select the “Project” category from the list on the left.

This should show `TouchEngine` along any other custom plugins you’ve installed. Make sure the `Enabled` check box is checked, and you’ve successfully installed the plugin.

> 💡 Note: You should now restart Unreal Engine for the plugin changes to take effect.

You can now get started and use the plugin in Unreal Engine.

> Note: When packaging a project that uses the plugin, you can save packaging time by disabling unnecessary Windows shader targets in the project's targeted RHIs / shader formats. In practice, uncheck DX11 and Vulkan (SM5 and SM6) if you do not need them for your project.

* [Getting Started](getting-started.md)
