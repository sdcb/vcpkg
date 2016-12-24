# Common Ambient Variables:
#   VCPKG_ROOT_DIR = <C:\path\to\current\vcpkg>
#   TARGET_TRIPLET is the current triplet (x86-windows, etc)
#   PORT is the current port name (zlib, etc)
#   CURRENT_BUILDTREES_DIR = ${VCPKG_ROOT_DIR}\buildtrees\${PORT}
#   CURRENT_PACKAGES_DIR  = ${VCPKG_ROOT_DIR}\packages\${PORT}_${TARGET_TRIPLET}
#

include(vcpkg_common_functions)

set(SOURCE_PATH ${CURRENT_BUILDTREES_DIR}/src/anax-2.1.0)
vcpkg_download_distfile(ARCHIVE
    URLS "https://github.com/miguelmartin75/anax/archive/v2.1.0.zip"
    FILENAME "anax-2.1.0.zip"
    SHA512 89f2df64add676ab48a19953b95d8eae1da9c8c5f3c0f6bc757a3bc99af6e4360c56c12d27d12c672ccd754b1f53a5e271533b381641f20e8cf3ca8ddda6cd1a
)
vcpkg_extract_source_archive(${ARCHIVE})

vcpkg_apply_patches(
    SOURCE_PATH ${SOURCE_PATH}
    PATCHES ${CMAKE_CURRENT_LIST_DIR}/Add-bin-output.patch
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
	OPTIONS
	-DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=ON 
)

vcpkg_install_cmake()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)

# Handle copyright
file(COPY ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/anax)
file(RENAME ${CURRENT_PACKAGES_DIR}/share/anax/LICENSE ${CURRENT_PACKAGES_DIR}/share/anax/copyright)
