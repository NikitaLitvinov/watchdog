cmake_minimum_required(VERSION 3.16)
project(simple_watchdog)

SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/bin
        CACHE PATH
        "Single Directory for all"
        )

set(CMAKE_C_STANDARD 11)

add_subdirectory(src)
