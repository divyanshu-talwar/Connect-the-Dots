cmake_minimum_required(VERSION 3.6)
project(connect_the_dots)

set(CMAKE_MODULE_PATH  ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
include(HelperFunctions)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${PROJECT_SOURCE_DIR}/bin)

include_directories("ext/nanogui/include"
	"ext/nanogui/ext/eigen"
	"ext/nanogui/ext/nanovg/src"
        "src"
        )
option(NANOGUI_BUILD_EXAMPLE OFF)
#option(NANOGUI_BUILD_SHARED ON)
option(NANOGUI_BUILD_PYTHON OFF)
option(NANOGUI_INSTALL OFF)
add_subdirectory(ext/nanogui)

set(PLATFORM_LINK )
if (UNIX)
    set(PLATFORM_LINK -ldl)
endif (UNIX)
file(GLOB SOURCE_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(connect_the_dots ${SOURCE_FILES})
target_link_libraries(connect_the_dots nanogui ${NANOGUI_LIBRARIES} ${PLATFORM_LINK})
pre_build(connect_the_dots
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/res ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res)
