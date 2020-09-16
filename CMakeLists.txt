cmake_minimum_required(VERSION 3.12)

project(tiff_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-fpermissive -g3 -O0 -Wall -Wextra")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_custom_target(exec
    COMMAND ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}
    DEPENDS ${CMAKE_PROJECT_NAME}
    )

add_custom_target(copy_compile_commands ALL
    COMMAND cp ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_SOURCE_DIR}
    # DEPENDS tiff2ppm
    )

add_executable(tiff2ppm
    examples/tiff2ppm.cpp
    src/tiff_reader.cpp
    src/tiff_pal.cpp
    )

target_include_directories(tiff2ppm
    PRIVATE ./inc
    )

