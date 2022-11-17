# Website: https://software.intel.com/content/www/us/en/develop/articles/pin-a-binary-instrumentation-tool-downloads.html
# License: https://software.intel.com/sites/landingpage/pintool/pinlicense.txt
# This snippet: https://gist.github.com/mrexodia/f61fead0108603d04b2ca0ab045e0952
# chiro: supporting linux

# Thanks to Francesco for showing me this method
CPMAddPackage(
        NAME IntelPIN
        VERSION 3.24
        URL https://github.com/chiro2001/binary/releases/download/pintool/pin-3.24-98612-g6bd5931f2-gcc-linux.tar.gz
        DOWNLOAD_ONLY ON
)

if (IntelPIN_ADDED)
    message(STATUS "IntelPIN_SOURCE_DIR = ${IntelPIN_SOURCE_DIR}")
    # Automatically detect the subfolder in the zip
    # file(GLOB PIN_DIR LIST_DIRECTORIES true ${IntelPIN_SOURCE_DIR}/pin-*)
    set(PIN_DIR "${IntelPIN_SOURCE_DIR}")

    # Loosely based on ${PIN_DIR}/source/tools/Config/makefile.win.config
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        # set(PIN_EXE "${PIN_DIR}/intel64/bin/pin${CMAKE_EXECUTABLE_SUFFIX}")
        set(PIN_EXE "${PIN_DIR}/intel64/bin/pinbin")
    else ()
        # set(PIN_EXE "${PIN_DIR}/ia32/bin/pin${CMAKE_EXECUTABLE_SUFFIX}")
        set(PIN_EXE "${PIN_DIR}/ia32/bin/pinbin")
    endif ()
    # string(REGEX REPLACE "/" "\\\\" PIN_EXE ${PIN_EXE})

    add_library(IntelPIN INTERFACE)

    target_include_directories(IntelPIN INTERFACE
            ${PIN_DIR}/source/include/pin
            ${PIN_DIR}/source/include/pin/gen
            ${PIN_DIR}/extras/components/include
            ${PIN_DIR}/extras/stlport/include
            ${PIN_DIR}/extras
            ${PIN_DIR}/extras/libstdc++/include
            ${PIN_DIR}/extras/crt/include
            ${PIN_DIR}/extras/crt
            ${PIN_DIR}/extras/crt/include/kernel/uapi
            ${PIN_DIR}/extras/crt/include/kernel/uapi/asm-x86
            )

    target_link_libraries(IntelPIN INTERFACE
            pin
            xed
            m-dynamic
            c-dynamic
            unwind-dynamic
            # for icc:
            # imf intlc irng svml
            )

    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        # target_link_options(IntelPIN INTERFACE /NODEFAULTLIB /EXPORT:main /BASE:0xC5000000 /ENTRY:Ptrace_DllMainCRTStartup /IGNORE:4210 /IGNORE:4281)
        target_link_options(IntelPIN INTERFACE -nostartfiles -nodefaultlibs -nostdlib -shared)
    else ()
        # target_link_options(IntelPIN INTERFACE /NODEFAULTLIB /EXPORT:main /BASE:0x55000000 /ENTRY:Ptrace_DllMainCRTStartup@12 /IGNORE:4210 /IGNORE:4281 /SAFESEH:NO)
        target_link_options(IntelPIN INTERFACE -nostartfiles -nodefaultlibs -nostdlib -shared)
    endif ()

    # target_compile_options(IntelPIN INTERFACE /GR- /GS- /EHs- /EHa- /fp:strict /Oi- /FIinclude/msvc_compat.h /wd5208)

    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_include_directories(IntelPIN INTERFACE
                ${PIN_DIR}/extras/xed-intel64/include/xed
                ${PIN_DIR}/extras/crt/include/arch-x86_64
                )
        target_link_directories(IntelPIN INTERFACE
                ${PIN_DIR}/intel64/lib
                ${PIN_DIR}/intel64/lib-ext
                ${PIN_DIR}/intel64/runtime/pincrt
                ${PIN_DIR}/extras/xed-intel64/lib
                )
        target_compile_definitions(IntelPIN INTERFACE
                TARGET_IA32E
                HOST_IA32E
                TARGET_LINUX
                __PIN__=1
                PIN_CRT=1
                __LP64__
                )
        target_link_libraries(IntelPIN INTERFACE
                ${PIN_DIR}/intel64/runtime/pincrt/crtbeginS.o
                ${PIN_DIR}/intel64/runtime/pincrt/crtendS.o
                )
    else ()
        target_include_directories(IntelPIN INTERFACE
                ${PIN_DIR}/extras/xed-ia32/include/xed
                ${PIN_DIR}/extras/crt/include/arch-x86
                )
        target_link_directories(IntelPIN INTERFACE
                ${PIN_DIR}/ia32/lib
                ${PIN_DIR}/ia32/lib-ext
                ${PIN_DIR}/ia32/runtime/pincrt
                ${PIN_DIR}/extras/xed-ia32/lib
                )
        target_compile_definitions(IntelPIN INTERFACE
                TARGET_IA32
                HOST_IA32
                TARGET_LINUX
                __PIN__=1
                PIN_CRT=1
                __i386__
                )
        target_link_libraries(IntelPIN INTERFACE
                ntdll-32
                kernel32
                ${PIN_DIR}/ia32/runtime/pincrt/crtbeginS.o
                )
    endif ()

    # Create a static library InstLib that is used in a lot of example pintools
    file(GLOB InstLib_SOURCES
            "${PIN_DIR}/source/tools/InstLib/*.cpp"
            "${PIN_DIR}/source/tools/InstLib/*.H"
            )
    add_library(InstLib STATIC EXCLUDE_FROM_ALL ${InstLib_SOURCES})
    target_include_directories(InstLib PUBLIC "${PIN_DIR}/source/tools/InstLib")
    target_link_libraries(InstLib PUBLIC IntelPIN)

    function(add_pintool target)
        add_library(${target} SHARED ${ARGN})
        target_include_directories(${target} PUBLIC "${PIN_DIR}/source/tools/InstLib")
        target_link_libraries(${target} PRIVATE IntelPIN)
    endfunction()
    function(add_pintool_exec target)
        add_executable(${target} ${ARGN})
        target_link_libraries(${target} PRIVATE IntelPIN)
    endfunction()
endif ()