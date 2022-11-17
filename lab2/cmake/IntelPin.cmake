# Grab from https://github.com/drop-stones/pin-cmake
# Reference URL: https://gist.github.com/mrexodia/f61fead0108603d04b2ca0ab045e0952

CPMAddPackage(
  NAME IntelPin
  VERSION 3.19
#  URL https://software.intel.com/sites/landingpage/pintool/downloads/pin-3.19-98425-gd666b2bee-gcc-linux.tar.gz
  URL https://github.com/chiro2001/binary/releases/download/pintool/pin-3.19-98425-gd666b2bee-gcc-linux.tar.gz
  DOWNLOAD_ONLY ON
)

if(IntelPin_ADDED)
  # Automatically detect the subfolder in the gz
  file(GLOB PIN_DIR LIST_DIRECTORIES true ${IntelPin_SOURCE_DIR})

  set(PIN_EXE "${PIN_DIR}/pin")

  add_library(IntelPin INTERFACE)

  # reference URL: https://software.intel.com/sites/landingpage/pintool/docs/98425/PinCRT/PinCRT.pdf
  # reference files: makefile.unix.config

  # Compilation
  target_include_directories(IntelPin INTERFACE
    ${PIN_DIR}/extras/components/include
    ${PIN_DIR}/source/include/pin
    ${PIN_DIR}/source/include/pin/gen
    ${PIN_DIR}/extras/xed-intel64/include/xed
    ${PIN_DIR}/source/tools/Utils
    ${PIN_DIR}/source/tools/InstLib
  )
  target_include_directories(IntelPin SYSTEM INTERFACE
    ${PIN_DIR}/extras/stlport/include
    ${PIN_DIR}/extras/libstdc++/include
    ${PIN_DIR}/extras/crt/include
    ${PIN_DIR}/extras/crt/include/arch-x86_64
    ${PIN_DIR}/extras/crt/include/kernel/uapi
    ${PIN_DIR}/extras/crt/include/kernel/uapi/asm-x86
  )
  
  target_compile_definitions(IntelPin INTERFACE
    TARGET_IA32E
    HOST_IA32E
    TARGET_LINUX
    __PIN__=1
    PIN_CRT=1
  )
  target_compile_options(IntelPin INTERFACE
    -Wall
    -Werror
    -Wno-unknown-pragmas
    # for all compilers
    -funwind-tables
    -fno-stack-protector
    -fasynchronous-unwind-tables
    -fomit-frame-pointer
    -fno-strict-aliasing
    # for g++ in linux
    -fno-exceptions
    -fno-rtti
    -fPIC
    -faligned-new
    # optimization
    -O3
  )

  # Linkage
  target_link_options(IntelPin INTERFACE
    # TOOL_LDFLAGS
    -Wl,--hash-style=sysv 
    -Wl,-Bsymbolic 

    -nostdlib
  )

  # This order has meanings.
  target_link_libraries(IntelPin INTERFACE
    ${PIN_DIR}/intel64/runtime/pincrt/crtbeginS.o
    ${PIN_DIR}/intel64/runtime/pincrt/crtendS.o
    pin
    xed
    pin3dwarf
    dl-dynamic
    stlport-dynamic
    m-dynamic
    c-dynamic
    unwind-dynamic
  )
  target_link_directories(IntelPin INTERFACE
    ${PIN_DIR}/extras/components/lib/intel64
    ${PIN_DIR}/intel64/runtime/pincrt
    ${PIN_DIR}/intel64/lib
    ${PIN_DIR}/intel64/lib-ext
    ${PIN_DIR}/extras/xed-intel64/lib
  )

  function(add_pintool target)
    add_library(${target} SHARED ${ARGN})
    target_link_libraries(${target} PRIVATE IntelPin)
  endfunction()

  function(add_pintool_test target command paths)
    add_pintool(${target} ${paths})
    add_test(NAME run_${target}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${PIN_DIR}/pin -t ${CMAKE_BINARY_DIR}/lib${target}.so -- ${command})
  endfunction()

endif()