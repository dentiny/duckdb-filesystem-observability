# This file is included by DuckDB's build system. It specifies which extension
# to load

# Extension from this repo
duckdb_extension_load(observefs SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
                      EXTENSION_VERSION 0.6.0 LOAD_TESTS)

# Any extra extensions that should be built e.g.: duckdb_extension_load(json)

# Register ObserveFS when DuckDB and the extension unit tests link statically.
duckdb_extension_statically_link(observefs)
