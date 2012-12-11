TARGET_LIB_DIR = ../.libs

all: test-measure-time.recipe
test-measure-time.recipe: make_recipe.py $(TARGET_LIB_DIR)/libtargets.so.0 $(TARGET_LIB_DIR)/helper-bin
	$< measure-time > $@
