TARGET_LIB_DIR = ../.libs

all: test-measure-time.recipe
test-measure-time.recipe: make_recipe.py $(TARGET_LIB_DIR)/libtargets.so.0
	$< measure-time > $@
