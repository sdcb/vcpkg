if (VCPKG_LIBRARY_LINKAGE STREQUAL static)
    message(STATUS "Warning: Static building not supported yet. Building dynamic.") #Blocked by libbson
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
include(vcpkg_common_functions)
set(SOURCE_PATH ${CURRENT_BUILDTREES_DIR}/src/mongo-cxx-driver-r3.0.2)

vcpkg_download_distfile(ARCHIVE
    URLS "https://codeload.github.com/mongodb/mongo-cxx-driver/zip/r3.0.2"
    FILENAME "mongo-cxx-driver-r3.0.2.zip"
    SHA512 f3f1902df22ad58090ec2d4f22c9746d32b12552934d0eaf686b7e3b2e65ac9eeff9e28944cde75c5f5834735e8b76f879e1ca0e7095195f22e3ce6dd92b4524
)
vcpkg_extract_source_archive(${ARCHIVE})

vcpkg_apply_patches(
    SOURCE_PATH ${SOURCE_PATH}
    PATCHES ${CMAKE_CURRENT_LIST_DIR}/0001_cmake.patch
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
	OPTIONS
		-DLIBBSON_DIR=${CURRENT_INSTALLED_DIR}
		-DLIBMONGOC_DIR=${CURRENT_INSTALLED_DIR}
)

vcpkg_install_cmake()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/lib/cmake)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/lib/cmake)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)

file(COPY ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/mongo-cxx-driver)
file(RENAME ${CURRENT_PACKAGES_DIR}/share/mongo-cxx-driver/LICENSE ${CURRENT_PACKAGES_DIR}/share/mongo-cxx-driver/copyright)