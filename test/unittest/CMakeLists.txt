include_directories(include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/third_party)
include_directories(${CMAKE_SOURCE_DIR}/src/include)
include_directories(${CMAKE_SOURCE_DIR}/src/utils/include)
include_directories(${DuckDB_SOURCE_DIR}/third_party)
include_directories(${DuckDB_SOURCE_DIR}/test/include)

set(OBSERVEFS_UNITTEST_OBJECTS
    main.cpp test_filesystem_glob.cpp test_histogram.cpp test_no_destructor.cpp
    test_quantile_estimator.cpp test_string_utils.cpp)

add_executable(unittest_observefs ${OBSERVEFS_UNITTEST_OBJECTS})

if(NOT WIN32
   AND NOT SUN
   AND NOT ZOS)
  target_link_libraries(
    unittest_observefs
    test_helpers
    duckdb
    ${EXTENSION_NAME}
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl)
else()
  target_link_libraries(
    unittest_observefs
    test_helpers
    duckdb_static
    ${EXTENSION_NAME}
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl)
endif()

if(MINGW OR WIN32)
  target_link_libraries(unittest_observefs crypt32)
endif()
