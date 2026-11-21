# LVGL as one static library, shared by every language pack (run by
# the Makefile once LVGL has been fetched). Objects go to the target's
# output folder (arch.mk).

include arch.mk

LVGL_SRCS := $(shell find $(LVGL_DIR)/src -name '*.c')
LVGL_OBJS := $(patsubst $(LVGL_DIR)/%.c,$(OUT)/lvgl-obj/%.o,$(LVGL_SRCS))

$(LVGL_LIB): $(LVGL_OBJS)
	@rm -f $@
	@echo "AR  $@"
	@$(AR) rcs $@ $(LVGL_OBJS)

$(OUT)/lvgl-obj/%.o: $(LVGL_DIR)/%.c shim/lv_conf.h
	@mkdir -p $(dir $@)
	@echo "CC  $(notdir $<)"
	@$(CC) $(LVGL_CFLAGS) -c $< -o $@

-include $(LVGL_OBJS:.o=.d)
