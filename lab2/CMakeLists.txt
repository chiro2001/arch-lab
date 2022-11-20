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

add_subdirectory(${CMAKE_SOURCE_DIR}/corrosion)

find_program(CXXBRIDGE cxxbridge PATHS "$ENV{HOME}/.cargo/bin/")
if (CXXBRIDGE STREQUAL "CXXBRIDGE-NOTFOUND")
    message("Could not find cxxbridge, trying to install with `cargo install cxxbridge-cmd'")
    find_program(CARGO cargo PATHS "$ENV{HOME}/.cargo/bin/")
    if (CARGO STREQUAL "CARGO-NOTFOUND")
        message(FATAL_ERROR "Requires cargo available in path, install via rustup https://rustup.rs/")
    endif ()
    execute_process(COMMAND ${CARGO} install cxxbridge-cmd)
    find_program(CXXBRIDGE cxxbridge PATHS "$ENV{HOME}/.cargo/bin/")
endif ()

# cxxbridge for rust
corrosion_import_crate(MANIFEST_PATH src/rusty_code/Cargo.toml)
corrosion_add_cxxbridge(rusty_bridge CRATE rusty_code MANIFEST_PATH rusty_code FILES lib.rs)
target_link_options(rusty_bridge INTERFACE -Wl,--hash-style=sysv)

include_directories(${CMAKE_SOURCE_DIR}/src/include)

# Instruction counter example
add_pintool(icount ${PIN_DIR}/source/tools/InstLibExamples/icount.cpp)

add_pintool_test(pin_test "ls" ${CMAKE_SOURCE_DIR}/src/pin_test.cpp)

add_pintool_test(brchPredict "ls" ${CMAKE_SOURCE_DIR}/src/brchPredict.cpp)

add_pintool_test(bp "ls" ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_link_libraries(bp PRIVATE rusty_bridge)

add_executable(simple_test ${CMAKE_SOURCE_DIR}/src/simple_test.cpp)