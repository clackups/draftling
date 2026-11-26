# LVGL as one static library (run by the Makefile once LVGL has been
# fetched). Built with -pthread like everything else: a WebAssembly
# program with threads needs every object compiled for shared memory.

LVGL_SRCS := $(shell find $(LVGL_DIR)/src -name '*.c')
LVGL_OBJS := $(patsubst $(LVGL_DIR)/%.c,build/lvgl-obj/%.o,$(LVGL_SRCS))

$(LVGL_LIB): $(LVGL_OBJS)
	@rm -f $@
	@echo "AR  $@"
	@emar rcs $@ $(LVGL_OBJS)

build/lvgl-obj/%.o: $(LVGL_DIR)/%.c ../linux/shim/lv_conf.h
	@mkdir -p $(dir $@)
	@echo "CC  $(notdir $<)"
	@emcc $(LVGL_CFLAGS) -c $< -o $@

-include $(LVGL_OBJS:.o=.d)
