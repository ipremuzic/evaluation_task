cmake_minimum_required(VERSION 3.20.0)

set(ZEPHYR_EXTRA_MODULES $ENV{ZEPHYR_BASE}/../modules/googletest)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tst)

set(SOURCES
    app_test.cpp
    ./../src/middleware/middleware.cpp
    test_doubles/hal/hal_mock.cpp
)

target_sources(app_test PRIVATE ${SOURCES})

target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/hal ${CMAKE_CURRENT_LIST_DIR}/src/middleware)

target_link_libraries(app_test PUBLIC gtest_main)