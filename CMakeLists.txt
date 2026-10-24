cmake_minimum_required(VERSION 3.13)

set(ENV{BTSTACK_ROOT} "${CMAKE_CURRENT_SOURCE_DIR}/external/btstack")

if(NOT EXISTS "$ENV{BTSTACK_ROOT}/port/esp32/components/btstack/cmake/BTstack_project.cmake")
    message(FATAL_ERROR
        "BTstack is missing. Clone it into external/btstack and check out "
        "the revision documented in README.md.")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include($ENV{BTSTACK_ROOT}/port/esp32/components/btstack/cmake/BTstack_project.cmake)

project(ps4_bluepad32_web)
