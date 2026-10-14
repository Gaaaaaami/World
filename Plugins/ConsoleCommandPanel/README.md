# Console Command Panel

A dockable UE editor plugin that turns frequently used console commands into customizable, auto-saved one-click buttons, boosting your debugging and development efficiency.

## Features

- One-Click Execution: Run console commands instantly with a single click.
- Custom Commands: Add, edit, and delete your own command shortcuts.
- Smart Search: Filter your command list instantly by display name or actual command.
- Real-time Tooltips: Hover over any command to see its help text and parameters.
- Auto-Save: All commands are persisted to the editor config and survive restarts.
- Dockable Panel: Place the panel anywhere in your editor layout for quick access.

## Supported Engine

Developed and tested on Unreal Engine 5.8. Expected to work on UE 5.3 and above. Only 5.8 is officially verified.

## Installation

1. Copy the ConsoleCommandPanel folder into your project's Plugins directory.
2. Right-click your .uproject file and generate project files.
3. Compile your project.
4. Open the editor, go to Edit > Plugins, and enable Console Command Panel.
5. Restart the editor.

## Usage

- Click the plugin icon in the main toolbar to open the panel.
- Enter a display name and a console command, then click Add.
- Use the search box to filter commands.
- Hover over a command to see its help text and parameters.
- Click Run to execute, Edit to modify, Delete to remove.

## Default Commands

The plugin ships with a preset list of commonly used console commands (performance stats, rendering options, debug tools, etc.) located at:

Plugins/ConsoleCommandPanel/Config/DefaultConsoleCommandPanel.ini

On first launch, these commands are automatically loaded into the panel. You can freely edit, delete, or add new commands — your changes are saved to your project's config and will not be overwritten.

## Configuration

Your customized command list is saved to:

Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini

To reset to the default command list, delete the [/Script/ConsoleCommandPanel.ConsoleCommandPanelSettings] section in that file and restart the editor.

To change what the defaults are, edit the DefaultConsoleCommandPanel.ini inside the plugin's Config folder.

## Support

For bug reports or feature requests, contact: 3116885284@qq.com

## Copyright

Copyright 2026 GlimmerWorks Studio. All Rights Reserved.