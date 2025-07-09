# The TouchEngine for UE Plugin

## TouchEngine Plugin for Unreal Engine

* **Current Unreal Engine version:** `5.6.x`
* **Current TouchEngine version:** `2023.12000+`
* **Current Plugin version:** `1.6.0`

TouchEngine allows the use of TouchDesigner components in Unreal Engine. The plugin currently supports most Custom Parameters, CHOP inputs/outputs, TOP inputs/outputs and DAT inputs/outputs.

TouchEngine requires an installed version of TouchDesigner and any paid license: TouchDesigner or TouchPlayer (Commercial/Pro/Educational). TouchEngine does not work with Non-Commercial licenses. We recommend using the latest official version of TouchDesigner: [available here 🔗.](https://derivative.ca/download)

This repository is covering the TouchEngine For UE Plugin, for samples and samples documentation of the TouchEngine For UE Plugin, follow [this link 🔗](https://github.com/TouchDesigner/TouchEngine-UE-Samples/).

## Installation

Go to the Unreal Engine project directory for which you want to install the plugin. You should see at the root a `.uproject` file. If your project does not already have a `Plugins` folder, create a folder called `Plugins` within that directory. Place the `TouchEngine` folder into the Plugins folder.

> 💡 Note: If you cloned the repository, it is likely that the folder will be named TouchEngine-UE by default.
> Rename it to TouchEngine or clone it as TouchEngine. The easiest approach is to grab the release [from the Releases page 🔗](https://github.com/TouchDesigner/TouchEngine-UE/releases).

After you’ve followed these steps, to be sure that you’ve installed the plugin correctly, do the following:

- Open the project in Unreal Engine
- Under Edit -> Plugins, select the “Project” category from the list on the left.

This should show `TouchEngine` along any other custom plugins you’ve installed. Make sure the `Enabled` check box is checked, and you’ve successfully installed the plugin.

> 💡 Note: You should now restart Unreal Engine for the plugin changes to take effect.

You can now get started and use the plugin in Unreal Engine.

* [Getting Started](docs/getting-started.md)
* [Main features](docs/main-features.md)
* [How To-s](docs/how-tos.md)
* [Sync Modes](docs/sync-modes.md)
* [Samples Project](https://github.com/TouchDesigner/TouchEngine-UE-Samples/)
* [FAQ](docs/FAQ.md)
