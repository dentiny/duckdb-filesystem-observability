cmake_minimum_required(VERSION 3.5)

# Set extension name here
set(TARGET_NAME observefs)

set(CMAKE_CXX_STANDARD 14)

# DuckDB's extension distribution supports vcpkg. As such, dependencies can be
# added in ./vcpkg.json and then used in cmake with find_package. Feel free to
# remove or replace with other dependencies. Note that it should also be removed
# from vcpkg.json to prevent needlessly installing it..
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)

project(${TARGET_NAME})
include_directories(src/include)
include_directories(duckdb-httpfs/src/include)
include_directories(duckdb/third_party/httplib)

set(EXTENSION_SOURCES
    src/external_file_cache_query_function.cpp
    src/external_file_cache_stats_recorder.cpp
    src/fake_filesystem.cpp
    src/filesystem_ref_registry.cpp
    src/filesystem_status_query_function.cpp
    src/histogram.cpp
    src/metrics_collector.cpp
    src/numeric_utils.cpp
    src/observefs_extension.cpp
    src/observability_filesystem.cpp
    src/operation_latency_collector.cpp
    src/operation_size_collector.cpp
    src/quantile.cpp
    src/quantilelite.cpp
    src/quantile_estimator.cpp
    src/string_utils.cpp
    src/time_utils.cpp
    duckdb-httpfs/src/create_secret_functions.cpp
    duckdb-httpfs/src/crypto.cpp
    duckdb-httpfs/src/hash_functions.cpp
    duckdb-httpfs/src/hffs.cpp
    duckdb-httpfs/src/http_state.cpp
    duckdb-httpfs/src/httpfs.cpp
    duckdb-httpfs/src/httpfs_curl_client.cpp
    duckdb-httpfs/src/httpfs_extension.cpp
    duckdb-httpfs/src/httpfs_httplib_client.cpp
    duckdb-httpfs/src/s3fs.cpp)

build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})

# Link OpenSSL in both the static library as the loadable extension
target_link_libraries(${EXTENSION_NAME} OpenSSL::SSL OpenSSL::Crypto
                      CURL::libcurl)
target_link_libraries(${LOADABLE_EXTENSION_NAME} OpenSSL::SSL OpenSSL::Crypto
                      CURL::libcurl)

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")

# Test cases.
include_directories(duckdb/third_party/catch)

add_executable(test_histogram unit/test_histogram.cpp)
target_link_libraries(test_histogram ${EXTENSION_NAME})

add_executable(test_no_destructor unit/test_no_destructor.cpp)
target_link_libraries(test_no_destructor ${EXTENSION_NAME})

add_executable(test_string_utils unit/test_string_utils.cpp)
target_link_libraries(test_string_utils ${EXTENSION_NAME})

add_executable(test_quantile_estimator unit/test_quantile_estimator.cpp)
target_link_libraries(test_quantile_estimator ${EXTENSION_NAME})
