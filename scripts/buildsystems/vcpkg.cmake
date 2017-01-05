if(NOT VCPKG_TOOLCHAIN)
    if(CMAKE_GENERATOR_PLATFORM MATCHES "^[Ww][Ii][Nn]32$")
        set(_VCPKG_TARGET_TRIPLET_ARCH x86)
    elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Xx]64$")
        set(_VCPKG_TARGET_TRIPLET_ARCH x64)
    elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]$")
        set(_VCPKG_TARGET_TRIPLET_ARCH arm)
    else()
        if(CMAKE_GENERATOR MATCHES "^Visual Studio 14 2015 Win64$")
            set(_VCPKG_TARGET_TRIPLET_ARCH x64)
        elseif(CMAKE_GENERATOR MATCHES "^Visual Studio 14 2015 ARM$")
            set(_VCPKG_TARGET_TRIPLET_ARCH arm)
        elseif(CMAKE_GENERATOR MATCHES "^Visual Studio 14 2015$")
            set(_VCPKG_TARGET_TRIPLET_ARCH x86)
        elseif(CMAKE_GENERATOR MATCHES "^Visual Studio 15 2017 Win64$")
            set(_VCPKG_TARGET_TRIPLET_ARCH x64)
        elseif(CMAKE_GENERATOR MATCHES "^Visual Studio 15 2017 ARM")
            set(_VCPKG_TARGET_TRIPLET_ARCH arm)
        elseif(CMAKE_GENERATOR MATCHES "^Visual Studio 15 2017")
            set(_VCPKG_TARGET_TRIPLET_ARCH x86)
        else()
            find_program(_VCPKG_CL cl)
            if(_VCPKG_CL MATCHES "amd64/cl.exe$" OR _VCPKG_CL MATCHES "x64/cl.exe$")
                set(_VCPKG_TARGET_TRIPLET_ARCH x64)
            elseif(_VCPKG_CL MATCHES "arm/cl.exe$")
                set(_VCPKG_TARGET_TRIPLET_ARCH arm)
            elseif(_VCPKG_CL MATCHES "bin/cl.exe$" OR _VCPKG_CL MATCHES "x86/cl.exe$")
                set(_VCPKG_TARGET_TRIPLET_ARCH x86)
            else()
                message(FATAL_ERROR "Unable to determine target architecture.")
            endif()
        endif()
    endif()

    if(WINDOWS_STORE OR WINDOWS_PHONE)
        set(_VCPKG_TARGET_TRIPLET_PLAT uwp)
    else()
        set(_VCPKG_TARGET_TRIPLET_PLAT windows)
    endif()

    set(VCPKG_TARGET_TRIPLET ${_VCPKG_TARGET_TRIPLET_ARCH}-${_VCPKG_TARGET_TRIPLET_PLAT} CACHE STRING "Vcpkg target triplet (ex. x86-windows)")
    set(_VCPKG_INSTALLED_DIR ${CMAKE_CURRENT_LIST_DIR}/../../installed)
    set(_VCPKG_TOOLCHAIN_DIR ${CMAKE_CURRENT_LIST_DIR})

    if(CMAKE_BUILD_TYPE MATCHES "^Debug$" OR NOT DEFINED CMAKE_BUILD_TYPE)
        list(APPEND CMAKE_PREFIX_PATH
            ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug
        )
        list(APPEND CMAKE_LIBRARY_PATH
            ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib/manual-link
        )
    endif()
    list(APPEND CMAKE_PREFIX_PATH
        ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}
    )
    list(APPEND CMAKE_LIBRARY_PATH
        ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/manual-link
    )

    set(CMAKE_PROGRAM_PATH ${CMAKE_PROGRAM_PATH} ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools)

    option(VCPKG_APPLOCAL_DEPS "Automatically copy dependencies into the output directory for executables." ON)
    function(add_executable name)
        _add_executable(${ARGV})
        list(FIND ARGV "IMPORTED" IMPORTED_IDX)
        if(IMPORTED_IDX EQUAL -1)
            if(VCPKG_APPLOCAL_DEPS)
                add_custom_command(TARGET ${name} POST_BUILD
                    COMMAND powershell -noprofile -executionpolicy UnRestricted -file ${_VCPKG_TOOLCHAIN_DIR}/msbuild/applocal.ps1
                        -targetBinary $<TARGET_FILE:${name}>
                        -installedDir "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}$<$<CONFIG:Debug>:/debug>/bin"
                        -OutVariable out
                )
            endif()
            set_target_properties(${name} PROPERTIES VS_GLOBAL_VcpkgEnabled false)
        endif()
    endfunction()

    function(add_library name)
        _add_library(${ARGV})
        list(FIND ARGV "IMPORTED" IMPORTED_IDX)
        list(FIND ARGV "INTERFACE" INTERFACE_IDX)
        list(FIND ARGV "ALIAS" ALIAS_IDX)
        if(IMPORTED_IDX EQUAL -1 AND INTERFACE_IDX EQUAL -1 AND ALIAS_IDX EQUAL -1)
            set_target_properties(${name} PROPERTIES VS_GLOBAL_VcpkgEnabled false)
        endif()
    endfunction()

    set(VCPKG_TOOLCHAIN ON)
endif()
