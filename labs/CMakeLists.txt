cmake_minimum_required(VERSION 3.23)

project(branch_predication CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_CXX_STANDARD 17)

enable_testing()

set(CPM_DOWNLOAD_VERSION 0.36.0)

if (CPM_SOURCE_CACHE)
    set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif (DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else ()
    set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif ()

# Expand relative path. This is important if the provided path contains a tilde (~)
get_filename_component(CPM_DOWNLOAD_LOCATION ${CPM_DOWNLOAD_LOCATION} ABSOLUTE)
if (NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD
            https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
            ${CPM_DOWNLOAD_LOCATION}
            )
endif ()

include(${CPM_DOWNLOAD_LOCATION})

# Enable solution folder support
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Include Intel PIN targets (this should use find_package but this is easier sometimes)
include(cmake/IntelPin.cmake)

add_compile_options(-O3)

CPMAddPackage(
        NAME debug_macros
        VERSION 0.6
        GITHUB_REPOSITORY chiro2001/debug-macros)

link_libraries(debug_macros)
target_compile_definitions(debug_macros INTERFACE -DCONFIG_DEBUG=1)

# Instruction counter example
add_pintool(icount ${PIN_DIR}/source/tools/InstLibExamples/icount.cpp)

add_pintool_test(pin_test "ls" ${CMAKE_SOURCE_DIR}/src/pin_test.cpp)

add_pintool_test(brchPredict "ls" ${CMAKE_SOURCE_DIR}/src/brchPredict.cpp)

add_pintool_test(cacheModel "pwd" ${CMAKE_SOURCE_DIR}/src/cacheModel.cpp)

add_test(NAME brchPredict-coremark
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${PIN_DIR}/pin -t ${CMAKE_BINARY_DIR}/libbrchPredict.so -- ${PROJECT_SOURCE_DIR}/coremark.exe)

add_executable(simple_test ${CMAKE_SOURCE_DIR}/src/simple_test.cpp)