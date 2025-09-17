PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Configuration of extension
EXT_NAME=cache_httpfs
EXT_CONFIG=${PROJ_DIR}extension_config.cmake

# Include the Makefile from extension-ci-tools
include extension-ci-tools/makefiles/duckdb_extension.Makefile

format-all: format
	find unit/ -iname *.hpp -o -iname *.cpp | xargs clang-format --sort-includes=0 -style=file -i
	find benchmark/ -iname *.hpp -o -iname *.cpp | xargs clang-format --sort-includes=0 -style=file -i
	cmake-format -i CMakeLists.txt

test_unit: all
	find build/release/extension/observefs/ -type f -name "test*" -not -name "*.o" -not -name "*.cpp" -not -name "*.d" -exec {} \;

test_reldebug_unit: all
	find build/reldebug/extension/observefs/ -type f -name "test*" -not -name "*.o" -not -name "*.cpp" -not -name "*.d" -exec {} \;

test_debug_unit: debug
	find build/debug/extension/observefs/ -type f -name "test*" -not -name "*.o" -not -name "*.cpp" -not -name "*.d" -exec {} \;

PHONY: format-all test_unit test_debug_unit
