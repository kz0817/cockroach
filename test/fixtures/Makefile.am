TARGET_LIB_DIR = ../.libs

RECIPES = \
test-measure-time.recipe test-user-probe.recipe \
test-measure-time-target-exe.recipe test-measure-time-no-target-exe.recipe \
test-measure-time-target-exe-abs.recipe \
test-measure-time-no-target-exe-abs.recipe 

all: $(RECIPES)
test-measure-time.recipe: make_recipe.py $(TARGET_LIB_DIR)/libtargets.so.0 $(TARGET_LIB_DIR)/target-exe $(TARGET_LIB_DIR)/libimplicitdlopener.so.0
	$< measure-time > $@ || (rm -f $@; exit 1)

test-measure-time-target-exe.recipe: make_recipe.py $(TARGET_LIB_DIR)/libtargets.so.0 $(TARGET_LIB_DIR)/target-exe
	$< measure-time-target-exe > $@ || (rm -f $@; exit 1)

test-measure-time-no-target-exe.recipe: make_recipe.py $(TARGET_LIB_DIR)/libtargets.so.0 $(TARGET_LIB_DIR)/target-exe
	$< measure-time-no-target-exe > $@ || (rm -f $@; exit 1)

test-measure-time-target-exe-abs.recipe: make_recipe.py $(TARGET_LIB_DIR)/libtargets.so.0 $(TARGET_LIB_DIR)/target-exe
	$< measure-time-target-exe-abs > $@ || (rm -f $@; exit 1)

test-measure-time-no-target-exe-abs.recipe: make_recipe.py $(TARGET_LIB_DIR)/libtargets.so.0 $(TARGET_LIB_DIR)/target-exe
	$< measure-time-no-target-exe-abs > $@ || (rm -f $@; exit 1)

test-user-probe.recipe: make_recipe.py $(TARGET_LIB_DIR)/libtargets.so.0
	$< user-probe > $@ || (rm -f $@; exit 1)

.PHONY clean:
clean:
	rm -f $(RECIPES)
