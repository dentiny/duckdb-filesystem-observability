cmake_minimum_required(VERSION 3.5...3.29)

# Set extension name here
set(TARGET_NAME observefs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Override DuckDB encryption library, mimics httpfs extension behavior.
if(NOT EMSCRIPTEN)
  add_definitions(-DOVERRIDE_ENCRYPTION_UTILS=1)
else()
  set(DUCKDB_EXTENSION_HTTPFS_LINKED_LIBS
      "../../third_party/mbedtls/libduckdb_mbedtls.a")
endif()

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
    src/observability_filesystem.cpp
    src/observefs_extension.cpp
    src/observefs_functions.cpp
    src/observefs_instance_state.cpp
    src/operation_latency_collector.cpp
    src/operation_size_collector.cpp
    src/quantile.cpp
    src/quantilelite.cpp
    src/quantile_estimator.cpp
    src/string_utils.cpp
    src/time_utils.cpp
    duckdb-httpfs/src/create_secret_functions.cpp
    duckdb-httpfs/src/crypto.cpp
    duckdb-httpfs/src/hffs.cpp
    duckdb-httpfs/src/httpfs_extension.cpp
    duckdb-httpfs/src/http/http_request.cpp
    duckdb-httpfs/src/http/http_request_session.cpp
    duckdb-httpfs/src/http/http_settings.cpp
    duckdb-httpfs/src/http/http_state.cpp
    duckdb-httpfs/src/http/httpfs.cpp
    duckdb-httpfs/src/http/httpfs_certificates.cpp
    duckdb-httpfs/src/http/httpfs_curl_client.cpp
    duckdb-httpfs/src/http/httpfs_httplib_client.cpp
    duckdb-httpfs/src/http/httpfs_transport.cpp
    duckdb-httpfs/src/s3/s3_auth.cpp
    duckdb-httpfs/src/s3/s3_delete.cpp
    duckdb-httpfs/src/s3/s3_endpoint.cpp
    duckdb-httpfs/src/s3/s3_list.cpp
    duckdb-httpfs/src/s3/s3_provider.cpp
    duckdb-httpfs/src/s3/s3_request.cpp
    duckdb-httpfs/src/s3/s3_settings.cpp
    duckdb-httpfs/src/s3/s3_upload_session.cpp
    duckdb-httpfs/src/s3/s3_url.cpp
    duckdb-httpfs/src/s3/s3_xml_response.cpp
    duckdb-httpfs/src/s3/s3fs.cpp)

build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})

# Link OpenSSL in both the static library as the loadable extension
target_link_libraries(${EXTENSION_NAME} OpenSSL::SSL OpenSSL::Crypto
                      CURL::libcurl)
target_link_libraries(${LOADABLE_EXTENSION_NAME} OpenSSL::SSL OpenSSL::Crypto
                      CURL::libcurl)

if(APPLE)
  find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
  find_library(SECURITY_FRAMEWORK Security REQUIRED)
  target_link_libraries(${EXTENSION_NAME} ${COREFOUNDATION_FRAMEWORK}
                        ${SECURITY_FRAMEWORK})
  target_link_libraries(${LOADABLE_EXTENSION_NAME} ${COREFOUNDATION_FRAMEWORK}
                        ${SECURITY_FRAMEWORK})
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${EXTENSION_NAME} PRIVATE -Werror=thread-safety)
  target_compile_options(${LOADABLE_EXTENSION_NAME}
                         PRIVATE -Werror=thread-safety)
endif()

# Link Windows Cryptography API library for certificate loading on Windows
if(MINGW OR WIN32)
  target_link_libraries(${EXTENSION_NAME} crypt32)
  target_link_libraries(${LOADABLE_EXTENSION_NAME} crypt32)
endif()

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")

# Unit tests, disabled on Windows due to linking issues.
if(BUILD_UNITTESTS AND NOT WIN32)
  add_subdirectory(test/unittest)
endif()
