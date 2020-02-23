cmake_minimum_required(VERSION 3.10)

project(libael_openssl
	VERSION 0.1.0
	DESCRIPTION "openssl filter for the libael library"
	LANGUAGES CXX)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

include(GNUInstallDirs)

include(InstallRequiredSystemLibraries)
set(CPACK_PROJECT_URL "https://github.com/TomerHeber/libael_openssl")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VERSION_MAJOR "${libael_openssl_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${libael_openssl_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${libael_openssl_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT "heber.tomer@gmail.com")
set(CPACK_PACKAGE_VENDOR, "Tomer Heber")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
include(CPack)

configure_file(config.h.in include/config.h)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options(-Wall -Wextra -pedantic -g -O2)
endif()

find_package(OpenSSL 1.1.0 REQUIRED)
find_package(libael 0.2.0 REQUIRED)

add_subdirectory(lib)

include(CTest)
if(BUILD_TESTING)
	add_subdirectory(tests)
endif()

install(EXPORT libael_openssl_targets
	FILE libael_openssl_targets.cmake
	DESTINATION lib/cmake/libael_openssl)

include(CMakePackageConfigHelpers)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/libaelOpenSSLConfig.cmake"
	INSTALL_DESTINATION "lib/cmake/libael_openssl"
	NO_SET_AND_CHECK_MACRO
	NO_CHECK_REQUIRED_COMPONENTS_MACRO)

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/libaelOpenSSLConfigVersion.cmake"
	VERSION "${libael_openssl_VERSION_MAJOR}.${libael_openssl_VERSION_MINOR}.${libael_openssl_VERSION_PATCH}"
	COMPATIBILITY AnyNewerVersion)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/libaelOpenSSLConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/libaelOpenSSLConfigVersion.cmake
	DESTINATION lib/cmake/libael_openssl)