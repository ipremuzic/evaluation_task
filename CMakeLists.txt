cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(evaluation_task)

target_sources(app PRIVATE src/main.cpp src/middleware/middleware.cpp src/hal/hal.cpp)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/hal ${CMAKE_CURRENT_LIST_DIR}/src/middleware)
