# Grab from https://github.com/drop-stones/pin-cmake
# Reference URL: https://gist.github.com/mrexodia/f61fead0108603d04b2ca0ab045e0952

CPMAddPackage(
        NAME IntelPin
        VERSION 3.24
        #  URL https://software.intel.com/sites/landingpage/pintool/downloads/pin-3.19-98425-gd666b2bee-gcc-linux.tar.gz
        URL https://github.com/chiro2001/binary/releases/download/pintool/pin-3.24-98612-g6bd5931f2-gcc-linux.tar.gz
        DOWNLOAD_ONLY ON
)

function(apply_pin_target target)
    # Compilation
    target_include_directories(${target} INTERFACE
            ${PIN_DIR}/extras/components/include
            ${PIN_DIR}/source/include/pin
            ${PIN_DIR}/source/include/pin/gen
            ${PIN_DIR}/extras/xed-intel64/include/xed
            ${PIN_DIR}/source/tools/Utils
            ${PIN_DIR}/source/tools/InstLib
            )
    target_include_directories(${target} SYSTEM INTERFACE
            ${PIN_DIR}/extras/stlport/include
            ${PIN_DIR}/extras/libstdc++/include
            ${PIN_DIR}/extras/crt/include
            ${PIN_DIR}/extras/crt/include/arch-x86_64
            ${PIN_DIR}/extras/crt/include/kernel/uapi
            ${PIN_DIR}/extras/crt/include/kernel/uapi/asm-x86
            )

    target_compile_definitions(${target} INTERFACE
            TARGET_IA32E
            HOST_IA32E
            TARGET_LINUX
            __PIN__=1
            PIN_CRT=1
            )
    target_compile_options(${target} INTERFACE
            -Wall
            -Werror
            -Wno-unknown-pragmas
            # for all compilers
            -funwind-tables
            -fno-stack-protector
            -fasynchronous-unwind-tables
            -fomit-frame-pointer
            -fno-strict-aliasing
            -Wno-dangling-pointer
            # for g++ in linux
            -fno-exceptions
            -fno-rtti
            -fPIC
            -faligned-new
            # optimization
            -O3
            )

    # Linkage
#g++ -Wl,--hash-style=sysv
#/opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/intel64/runtime/pincrt/crtbegin.o
#-Wl,-pie
#-Wl,-Bsymbolic
#-fabi-version=2
#-o obj-intel64/statica.exe
#    obj-intel64/statica.o
#-L/opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/intel64/runtime/pincrt
#-L/opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/intel64/lib
#-L/opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/intel64/lib-ext
#-L/opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/extras/xed-intel64/lib
#-lsapin -lxed /opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/intel64/runtime/pincrt/crtend.o
#-lpin3dwarf -ldl-dynamic -nostdlib -lc++ -lc++abi -lm-dynamic -lc-dynamic -lunwind-dynamic
#g++ -Wall -Werror -Wno-unknown-pragmas -DPIN_CRT=1 -fno-stack-protector -fno-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-rtti -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX -fabi-version=2 -faligned-new -I/opt/pin/pin-3.24-98612-g6bd
#5931f2-gcc-linux/source/include/pin -I/opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/source/include/pin/gen -isystem /opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/extras/cxx/include -isystem /opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/extras/crt/include
#-isystem /opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/extras/crt/include/arch-x86_64 -isystem /opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/extras/crt/include/kernel/uapi -isystem /opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/extras/crt/include/kernel/ua
#pi/asm-x86 -I/opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/extras/components/include -I/opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/extras/xed-intel64/include/xed -I/opt/pin/pin-3.24-98612-g6bd5931f2-gcc-linux/source/tools/Utils -I/opt/pin/pin-3.24-9861
#2-g6bd5931f2-gcc-linux/source/tools/InstLib -O3 -fomit-frame-pointer -fno-strict-aliasing  -Wno-dangling-pointer -c -o obj-intel64/inscount0.o inscount0.cpp
    target_link_options(${target} INTERFACE
            # TOOL_LDFLAGS
            -Wl,--hash-style=sysv
            -Wl,-pie
            -Wl,-Bsymbolic
            -fabi-version=2
            -nostdlib
            )

    # This order has meanings.
    target_link_libraries(${target} INTERFACE
            ${PIN_DIR}/intel64/runtime/pincrt/crtbeginS.o
            ${PIN_DIR}/intel64/runtime/pincrt/crtendS.o
            pin
            xed
            pin3dwarf
            dl-dynamic
            m-dynamic
            c-dynamic
            unwind-dynamic
            c++
            c++abi
            sapin
            )
    target_link_directories(${target} INTERFACE
            ${PIN_DIR}/extras/components/lib/intel64
            ${PIN_DIR}/intel64/runtime/pincrt
            ${PIN_DIR}/intel64/lib
            ${PIN_DIR}/intel64/lib-ext
            ${PIN_DIR}/extras/xed-intel64/lib)
endfunction()

if (IntelPin_ADDED)
    # Automatically detect the subfolder in the gz
    file(GLOB PIN_DIR LIST_DIRECTORIES true ${IntelPin_SOURCE_DIR})

    set(PIN_EXE "${PIN_DIR}/pin")

    add_library(IntelPin INTERFACE)

    # reference URL: https://software.intel.com/sites/landingpage/pintool/docs/98425/PinCRT/PinCRT.pdf
    # reference files: makefile.unix.config

    apply_pin_target(IntelPin)

    file(COPY_FILE ${CMAKE_SOURCE_DIR}/cmake/cstdint ${PIN_DIR}/extras/libstdc++/include/cstdint)

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

endif ()