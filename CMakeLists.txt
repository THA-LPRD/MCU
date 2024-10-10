# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(SDKCONFIG "${CMAKE_BINARY_DIR}/sdkconfig")
set(CONFIG_APP_PROJECT_VER "2.0.0-aplha")
set(IDF_TARGET "esp32s3")
set(CMAKE_CXX_STANDARD 23)

if(LPRD_RELEASE)
    set(SDKCONFIG_DEFAULTS "profiles/release/sdkconfig.release")
    add_definitions(-D_LPRD_RELEASE)
elseif(LPRD_DEBUG)
    set(SDKCONFIG_DEFAULTS "profiles/debug/sdkconfig.debug")
    add_definitions(-D_LPRD_DEBUG)
else()
    message(FATAL_ERROR "Unknown configuration: either LPRD_RELEASE or LPRD_DEBUG must be true.")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(LPRD-MCU)
