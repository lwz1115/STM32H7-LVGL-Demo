CSRCS += $(shell find -L $(LVGL_DIR)/$(LVGL_DIR_NAME)/GUI_APP/Player -name "*.c")

CFLAGS += -I$(LVGL_DIR)/$(LVGL_DIR_NAME)/GUI_APP/Player
CFLAGS += -I$(LVGL_DIR)/$(LVGL_DIR_NAME)/GUI_APP/Player/MainPage
CFLAGS += -I$(LVGL_DIR)/$(LVGL_DIR_NAME)/GUI_APP/Player/MainPage/assets
