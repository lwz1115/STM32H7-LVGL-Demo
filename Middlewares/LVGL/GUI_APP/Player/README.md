# Player Module

A modular UI framework for LVGL applications.

## Structure

```
Player/
©À©¤©¤ lv_player.h          # Main header file
©À©¤©¤ lv_player.mk         # Makefile configuration
©À©¤©¤ README.md            # This file
©¸©¤©¤ MainPage/            # Main screen module
    ©À©¤©¤ lv_player_main.h # Main page header
    ©¸©¤©¤ lv_player_main.c # Main page implementation
```

## Modules

### MainPage
The main screen module containing the primary UI components.

## Usage

```c
#include "lv_player.h"

// In your main function
lv_player();
```

## Adding New Screens

To add a new screen module:
1. Create a new folder under Player/ (e.g., SettingsPage/)
2. Add header and source files (e.g., lv_player_settings.h, lv_player_settings.c)
3. Update lv_player.h to include the new module

## Build Configuration

Add the following to your project Makefile:
```makefile
include $(LVGL_DIR)/$(LVGL_DIR_NAME)/GUI_APP/Player/lv_player.mk
```
