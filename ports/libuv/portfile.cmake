include(${CMAKE_TRIPLET_FILE})
if (VCPKG_LIBRARY_LINKAGE STREQUAL static)
    message(FATAL_ERROR "Static building not supported yet")
endif()
include(vcpkg_common_functions)
set(SOURCE_PATH ${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-src/libuv-1.9.1)
vcpkg_download_distfile(ARCHIVE
    URLS "https://github.com/libuv/libuv/archive/v1.9.1.zip"
    FILENAME "libuv-v1.9.1.zip"
    SHA512 3eb8711e3612fb3f5a1ddeb4614b2bec29c022ac5c6c2590bc5239825d758a73be0819c52747956a029859ef4e416bf3fce16665bac2c6c4890f736b47c38226
)

if(NOT EXISTS ${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-src)
    message(STATUS "Extracting source ${ARCHIVE}")
    file(MAKE_DIRECTORY ${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-src)
    vcpkg_execute_required_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${ARCHIVE}
        WORKING_DIRECTORY ${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-src
        LOGNAME extract-${TARGET_TRIPLET}
    )
endif()

find_program(PYTHON2
    NAMES python2 python
    PATHS C:/python27 ENV PYTHON
)
if(NOT PYTHON2 MATCHES "NOTFOUND")
    execute_process(
        COMMAND ${PYTHON2} --version
        OUTPUT_VARIABLE PYTHON_VER_CHECK_OUT
        ERROR_VARIABLE PYTHON_VER_CHECK_ERR
    )
    set(PYTHON_VER_CHECK "${PYTHON_VER_CHECK_OUT}${PYTHON_VER_CHECK_ERR}")
    debug_message("PYTHON_VER_CHECK=${PYTHON_VER_CHECK}")
    if(NOT PYTHON_VER_CHECK MATCHES "Python 2.7")
        set(PYTHON2 PYTHON2-NOTFOUND)
        find_program(PYTHON2
            NAMES python2 python
            PATHS C:/python27 ENV PYTHON
            NO_SYSTEM_ENVIRONMENT_PATH
        )
    endif()
endif()

if(PYTHON2 MATCHES "NOTFOUND")
    message(FATAL_ERROR "libuv uses the GYP build system, which requires Python 2.7.\n"
    "Python 2.7 was not found in the path or by searching inside C:\\Python27.\n"
    "There is no portable redistributable for Python 2.7, so you will need to install the MSI located at:\n"
    "    https://www.python.org/ftp/python/2.7.11/python-2.7.11.amd64.msi\n"
    )
endif()

set(ENV{GYP_MSVS_VERSION} 2015)
set(ENV{PYTHON} ${PYTHON2})

if(TRIPLET_SYSTEM_ARCH MATCHES "x86|x64")
    message(STATUS "Building Release")
    vcpkg_execute_required_process(
        COMMAND cmd /c vcbuild.bat release ${TRIPLET_SYSTEM_ARCH} shared
        WORKING_DIRECTORY ${SOURCE_PATH}
        LOGNAME ${TARGET_TRIPLET}-build-rel
    )
    message(STATUS "Building Debug")
    vcpkg_execute_required_process(
        COMMAND cmd /c vcbuild.bat debug ${TRIPLET_SYSTEM_ARCH} shared
        WORKING_DIRECTORY ${SOURCE_PATH}
        LOGNAME ${TARGET_TRIPLET}-build-dbg
    )
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

file(MAKE_DIRECTORY
    ${CURRENT_PACKAGES_DIR}/include
    ${CURRENT_PACKAGES_DIR}/lib
    ${CURRENT_PACKAGES_DIR}/bin
    ${CURRENT_PACKAGES_DIR}/debug/lib
    ${CURRENT_PACKAGES_DIR}/debug/bin
    ${CURRENT_PACKAGES_DIR}/share/libuv
    )

file(COPY
    ${SOURCE_PATH}/include/tree.h
    ${SOURCE_PATH}/include/uv.h
    ${SOURCE_PATH}/include/uv-version.h
    ${SOURCE_PATH}/include/uv-errno.h
    ${SOURCE_PATH}/include/uv-threadpool.h
    ${SOURCE_PATH}/include/uv-win.h
    DESTINATION ${CURRENT_PACKAGES_DIR}/include)

file(COPY
    ${SOURCE_PATH}/Debug/libuv.dll
    DESTINATION ${CURRENT_PACKAGES_DIR}/debug/bin)
file(COPY
    ${SOURCE_PATH}/Debug/libuv.lib
    DESTINATION ${CURRENT_PACKAGES_DIR}/debug/lib)
file(COPY
    ${SOURCE_PATH}/Release/libuv.dll
    DESTINATION ${CURRENT_PACKAGES_DIR}/bin)
file(COPY
    ${SOURCE_PATH}/Release/libuv.lib
    DESTINATION ${CURRENT_PACKAGES_DIR}/lib)
file(COPY
    ${SOURCE_PATH}/LICENSE
    DESTINATION ${CURRENT_PACKAGES_DIR}/share/libuv)

file(RENAME
    ${CURRENT_PACKAGES_DIR}/share/libuv/LICENSE
    ${CURRENT_PACKAGES_DIR}/share/libuv/copyright)
vcpkg_copy_pdbs()
