project(watchdog)

set(CMAKE_C_FLAGS "-Werror=all -Werror=extra -pedantic")
file(GLOB SOURCE_FILES *.c)

add_executable(watchdog ${SOURCE_FILES})
