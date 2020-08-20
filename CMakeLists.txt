#
#
# CMake compatibility issues: don't modify this, please!
cmake_minimum_required (VERSION 3.5)


#                                                
#                                **              
#                                 **             
#                                 **             
#                                 **             
#                   ****          **             
#   ***  ****      * ***  *   *** **      ***    
#    **** **** *  *   ****   *********   * ***   
#     **   ****  **    **   **   ****   *   ***  
#     **    **   **    **   **    **   **    *** 
#     **    **   **    **   **    **   ********  
#     **    **   **    **   **    **   *******   
#     **    **   **    **   **    **   **        
#     **    **    ******    **    **   ****    * 
#     ***   ***    ****      *****      *******  
#      ***   ***              ***        *****   
#                                                      
# get timestamp and build a patch level from the year and
# the day of the year. This is a unique number to distinguish
# different builds
string(TIMESTAMP THIS_YEAR "%Y")
# Patch level as year + day of the year
string(TIMESTAMP PATCH_LEVEL "%j")	# day of the year
math(EXPR PATCH_LEVEL "((${THIS_YEAR} - 2018) * 1000) + ${PATCH_LEVEL}")

#
# set project name/properties
#
project(NODE 
	VERSION 0.8.${PATCH_LEVEL}.1
	LANGUAGES CXX C
	DESCRIPTION "Smart Metering Framework")
#	HOMEPAGE_URL "https://solostec.ch")	- requires CMake 3.12.4


set (${PROJECT_NAME}_COPYRIGHT_YEAR ${THIS_YEAR})


message(STATUS "**                                                  ") 
message(STATUS "**                                **                ")  
message(STATUS "**                                 **               ")  
message(STATUS "**                                 **               ")  
message(STATUS "**                                 **               ")  
message(STATUS "**                   ****          **               ")  
message(STATUS "**   ***  ****      * ***  *   *** **      ***      ")  
message(STATUS "**    **** **** *  *   ****   *********   * ***     ")  
message(STATUS "**     **   ****  **    **   **   ****   *   ***    ")  
message(STATUS "**     **    **   **    **   **    **   **    ***   ")  
message(STATUS "**     **    **   **    **   **    **   ********    ")  
message(STATUS "**     **    **   **    **   **    **   *******     ")  
message(STATUS "**     **    **   **    **   **    **   **          ")  
message(STATUS "**     **    **    ******    **    **   ****    *   ")  
message(STATUS "**     ***   ***    ****      *****      *******    ")  
message(STATUS "**      ***   ***              ***        *****     ")  
message(STATUS "**")                                        


#
# Collect and dump some basic information
#
string(TIMESTAMP NOW_UTC "%Y-%m-%dT%H:%M:%SZ" UTC)
set (${PROJECT_NAME}_NOW_UTC ${NOW_UTC})

include(ProcessorCount)
ProcessorCount(${PROJECT_NAME}_CPU_COUNT)

if (${${PROJECT_NAME}_CPU_COUNT} LESS 4)
	set(${PROJECT_NAME}_POOL_SIZE 4)
else()
	set(${PROJECT_NAME}_POOL_SIZE ${${PROJECT_NAME}_CPU_COUNT})
endif()


message(STATUS "** CMake           : v${CMAKE_VERSION}")
message(STATUS "** Generator       : ${CMAKE_GENERATOR}")
message(STATUS "** Platform        : ${CMAKE_SYSTEM}")
message(STATUS "** System          : ${CMAKE_SYSTEM_NAME}")
message(STATUS "** Compiler        : ${CMAKE_CXX_COMPILER_ID} v${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "** Timestamp       : ${${PROJECT_NAME}_NOW_UTC}")
message(STATUS "** CPU Cores       : ${${PROJECT_NAME}_CPU_COUNT}")
message(STATUS "** Crosscompiling  : ${CMAKE_CROSSCOMPILING}")
message(STATUS "** Pool Size       : ${${PROJECT_NAME}_POOL_SIZE}")
message(STATUS "** Patchlevel      : ${PROJECT_VERSION_PATCH}")
if(UNIX)
# has no meaning on VS
# set default cmake build type to RelWithDebInfo (None Debug Release RelWithDebInfo MinSizeRel)
if( NOT CMAKE_BUILD_TYPE )
	set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build Type")
	message(STATUS "** Set build type  : ${CMAKE_BUILD_TYPE}")
endif()
message(STATUS "** Build type      : ${CMAKE_BUILD_TYPE}")
endif(UNIX)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(STATUS "** Address Model   : 64 bit")
	set(${PROJECT_NAME}_ADDRESS_MODEL 64)
	set(${PROJECT_NAME}_PREFERRED_BUFFER_SIZE 8192)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(STATUS "** Address Model   : 32 bit")
	set(${PROJECT_NAME}_ADDRESS_MODEL 32)
	set(${PROJECT_NAME}_PREFERRED_BUFFER_SIZE 4096)
else()
	message(STATUS "** Address Model   : not supported")
	set(${PROJECT_NAME}_ADDRESS_MODEL 16)
	set(${PROJECT_NAME}_PREFERRED_BUFFER_SIZE 2048)
endif()

#
#	Generate salt
#	7 hex chars generate an unsigned 32 bit integer
#
string(RANDOM LENGTH 7 ALPHABET "1234567890ABCDEF" __RND_VALUE_SALT)
set (${PROJECT_NAME}_SALT_VALUE 	"0x${__RND_VALUE_SALT}")
set (${PROJECT_NAME}_SALT_STRING 	${__RND_VALUE_SALT})

#
#	Generate password
#
string(RANDOM LENGTH 8 ALPHABET "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" __RND_VALUE_PWD)
set (${PROJECT_NAME}_PWD 	"${__RND_VALUE_PWD}")

#
# manage unit test: NODE_BUILD_TEST
# default is OFF
#
if(NOT DEFINED ${PROJECT_NAME}_BUILD_TEST)
	set(${PROJECT_NAME}_BUILD_TEST OFF CACHE BOOL "build unit test")
endif()

#
# manage cross compiling: NODE_CROSS_COMPILE
# default is OFF
#
if (NOT "${PROJECT_NAME}_CROSS_COMPILE")
    set(${PROJECT_NAME}_CROSS_COMPILE OFF CACHE BOOL "cross compiling")
endif()

#
#	The path to cmake modules
#
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/src/modules/")

#
#	setup cyng library
#   -DCYNG_ROOT:path=...
#	assume parallel installation - that is both projects share the same parent directory.
#
find_package(CYNG REQUIRED)

if(CYNG_FOUND)
    message(STATUS "** CYNG include path : ${CYNG_INCLUDE_DIRS}")
    message(STATUS "** CYNG Libraries    : ${CYNG_LIBRARIES}")
endif()

#
#	setup crypto library
#   -DCRYPT_ROOT:path=...
#	assume parallel installation - that is both projects share the same parent directory.
#
if(NOT CRYPT_ROOT)
    set(CRYPT_ROOT "${PROJECT_SOURCE_DIR}/../crypto" CACHE PATH "CRYPT_ROOT")
endif()
message(STATUS "** CRYPT_ROOT        : ${CRYPT_ROOT}")

get_filename_component(CRYPT_INCLUDE_DIR "${CRYPT_ROOT}/include" REALPATH)
message(STATUS "** CRYPT include path: ${CRYPT_INCLUDE_DIR}")

#
#	setup crypt build directory
#   -DCRYPT_BUILD:path=...
#	assume parallel installation - that is both projects share the same parent directory.
#
if (NOT CRYPT_BUILD)
	if ("${PROJECT_NAME}_CROSS_COMPILE")
		set(CRYPT_BUILD "${CRYPT_ROOT}/build/v5te" CACHE PATH "CRYPT_BUILD")
	else()
		set(CRYPT_BUILD "${CRYPT_ROOT}/build" CACHE PATH "CRYPT_BUILD")
	endif()
	message(STATUS "** Set CRYPT_BUILD   : ${CRYPT_BUILD}")
endif()

get_filename_component(CRYPT_LIBRARY_DIR ${CRYPT_BUILD} REALPATH)
message(STATUS "** CRYPT library path: ${CRYPT_LIBRARY_DIR}")

if (UNIX)
    set(SMF_CRYPTO_LIBRARY "${CRYPT_LIBRARY_DIR}/libcrypto.so")
    message(STATUS "** CRYPT Libraries   : ${CRYPT_LIBRARY_DIR}/libcrypto.so")
else()
	#
	#	$(ConfigurationName) is a variable used by the MS build system
	#
    set(SMF_CRYPTO_LIBRARY "${CRYPT_LIBRARY_DIR}/$(ConfigurationName)/crypto.lib")
    message(STATUS "** CRYPT Libraries    : ${CRYPT_LIBRARY_DIR}/crypto.lib")
endif()

#
#	setup compiler features
#   ToDo: currently only available if CYNG library is installed for developers
#
include(CPPFeatureSupport)

#
#	setup C++ compiler
#
if (CMAKE_COMPILER_IS_GNUCXX)

	#
	# gnu C++ select dialect
	#
	
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS  "4.7")
		set(CMAKE_CXX_STANDARD 11)
		message(STATUS "** C++ support     : C++11")
	elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS  "5.1")
	# default since  GCC 6.1
		set(CMAKE_CXX_STANDARD 14)
		message(STATUS "** C++ support     : C++14")
	elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS  "8.1")
    # partly since 5.0
		set(CMAKE_CXX_STANDARD 17)
		message(STATUS "** C++ support     : C++17")
	else()
    # partly since 8.0
		if(${CMAKE_VERSION} VERSION_LESS "3.12.4") 
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")			
			message(STATUS "** C++ support     : C++2a")
		else()
			set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard to be used")
			message(STATUS "** C++ support     : C++2a")
		endif()
		
		#	suppress messages about ABI incompatibilities
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")	
	endif()
	
	# only shared libraries
	set(GLOBAL_LIBRARY_TYPE SHARED)

	# since  CMake 3.1.0
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)
	
elseif(MSVC)

	#
	# Microsoft C++
	#
	
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "18.0")
	# 	prior Visual Studio 2013
		message(FATAL_ERROR "Insufficient MSVC version")
	endif()

	# only static libraries
	set(GLOBAL_LIBRARY_TYPE STATIC)

	#
	#	Target is Windows 10
	#
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /bigobj /std:c++latest /MP")	
	add_compile_definitions(_WIN32_WINNT=0x0A00 _SCL_SECURE_NO_WARNINGS _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE BOOST_BIND_NO_PLACEHOLDERS)
	
	#
	# help CMake to find OpenSSL
	#
	if(NOT OPENSSL_ROOT_DIR)
		set(OPENSSL_ROOT_DIR "C:/local/OpenSSL-Win64" CACHE PATH "OPENSSL_ROOT_DIR")
		message(STATUS "** Set OPENSSL_ROOT_DIR: ${OPENSSL_ROOT_DIR}")
	endif()
else()

	message( FATAL_ERROR "Unknown or missing compiler: ${CMAKE_CXX_COMPILER_ID}" )
	
endif()

#
#	add NODE_ADDRESS_MODEL=N (since CMake 3.12.4)
#
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.4") 
	add_compile_definitions(${PROJECT_NAME}_ADDRESS_MODEL=${${PROJECT_NAME}_ADDRESS_MODEL})
endif()

#
#	setup Boost library
#
include(SelectBoostLibrary)

#
#	setup SSL library
#
if(NOT DEFINED ${PROJECT_NAME}_SSL_SUPPORT)
	set(${PROJECT_NAME}_SSL_SUPPORT ON CACHE BOOL "SSL support")
endif()

if(${PROJECT_NAME}_SSL_SUPPORT)

    message(STATUS "** Search openSSL   : ${OPENSSL_ROOT_DIR}")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
        find_package(OpenSSL 1.0.2 REQUIRED)
    else()
        find_package(OpenSSL 1.0.1 REQUIRED)
    endif()
    if(OPENSSL_FOUND)

        add_definitions(-D${PROJECT_NAME}_SSL_INSTALLED)
        
    #	message(STATUS "** openSSL Found         : ${OPENSSL_FOUND}")
        message(STATUS "** openSSL Version       : ${OPENSSL_VERSION}")
        message(STATUS "** openSSL Include       : ${OPENSSL_INCLUDE_DIR}")
        message(STATUS "** openSSL crypto library: ${OPENSSL_CRYPTO_LIBRARY}")
        message(STATUS "** openSSL SSL library   : ${OPENSSL_SSL_LIBRARY}")
        message(STATUS "** openSSL Libraries     : ${OPENSSL_LIBRARIES}")
    
        set(${PROJECT_NAME}_SSL_VERSION ${OPENSSL_VERSION})
    #
    else()
            set(${PROJECT_NAME}_SSL_VERSION "unknown")
    endif()
    
else()

    message(WARNING "** no SSL support")
    
endif()

#
# libpugixml-dev (v1.9)
# On Windows set CMake variables PugiXML_INCLUDE_DIRS and PugiXML_LIBRARIES 
#
# Included as 3party software
#
set(PUGIXML_INCLUDE_DIR "${CYNG_ROOT_DEV}/lib/xml/pugixml/src")
message(STATUS "** PugiXML Include   : ${PUGIXML_INCLUDE_DIR}")
add_definitions(-D${PROJECT_NAME}_PUGIXML_INSTALLED)

#
# configure a header file to pass some of the CMake settings
# to the source code
#
configure_file (
  "${PROJECT_SOURCE_DIR}/src/main/templates/project_info.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h"
)

#
# minimal test program
# hello world!
# cross compile with 
# arm-linux-gnueabihf-g++ -O3 -g3 -Wall -fPIC -o "main.o" -c "main.cpp"
# arm-linux-gnueabihf-g++ -o "hello" main.o
#
include (test/hello/hello.cmake)
add_executable(hello ${hello})


#
#	all libraries
#
add_subdirectory(lib)

#
#	install libraries
#

#
#	all applications
#
add_subdirectory(nodes)

#
# test unit using Boost.Test
# BOOST_TEST_DYN_LINK is required to build a main() function
# cmake -DNODE_BUILD_TEST:bool=ON ..
#

if(${PROJECT_NAME}_BUILD_TEST AND ${PROJECT_NAME}_SSL_SUPPORT)
	add_subdirectory(test)
endif()

#
# SMF tool
#
if(NOT ${PROJECT_NAME}_CROSS_COMPILE)
	add_subdirectory(tools/smf)

endif()

# set(smf_link_libs smf_info smf_protocol_sml smf_protocol_mbus ${CYNG_LIBRARIES} ${OPENSSL_LIBRARIES})
# if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
# 	list(APPEND smf_link_libs "${Boost_LIBRARIES}")
# 	if (UNIX)
# 		list(APPEND smf_link_libs pthread readline)
# 	endif()
# endif()
# target_link_libraries(smf ${smf_link_libs})

#
#   Generate configuration files
#   Place service files in /etc/systemd/system/
#
#   some usefull commands:
#   systemctl list-unit-files --state=enabled
#   sudo systemctl daemon-reload
#   journalctl -f -u node-http
#
if(UNIX)

	include(generate_configuration_linux)

else()

	include(generate_configuration_windows)

endif(UNIX)

#
#   Generate documentation
#
include(GenerateDoxygenDoc)

#
# test unit using Boost.Test
# BOOST_TEST_DYN_LINK is required to build a main() function
#
# include (test/unit-test.cmake)
# add_executable(unit_test ${unit_test})
# target_link_libraries(unit_test
# 	cyng_core cyng_io cyng_async cyng_log cyng_store cyng_vm cyng_sql
# 	${Boost_LIBRARIES})
# 	
# # BOOST_TEST_DYN_LINK is required to build a main() function
# set_property(
# 	TARGET unit_test
# 	PROPERTY COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK)

#
# GeneratING OPKG files requires the OPKG tools (https://git.yoctoproject.org/cgit/cgit.cgi/opkg-utils)
# fakeroot opkg/opkg-tools/opkg-buildpackage
# Install with: opkg --force-space install /tmp/oecp-cyng_0.x_armel.ipk
#

if(${PROJECT_NAME}_CROSS_COMPILE)

	string(TOLOWER ${PROJECT_NAME} OECP_NAME)

	configure_file (
		  "${PROJECT_SOURCE_DIR}/src/main/templates/opkg/control.in"
		  "${PROJECT_BINARY_DIR}/opkg/control")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/src/main/templates/opkg/postinst.in"
		  "${PROJECT_BINARY_DIR}/opkg/postinst")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/src/main/templates/opkg/rules.in"
		  "${PROJECT_BINARY_DIR}/opkg/rules")
		  
endif()

install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/main/include/smf" # source directory
        DESTINATION "include" # target directory
        FILES_MATCHING # install only matched files
        PATTERN "*.h"
        PATTERN "*hpp"
)

set(NODE_TARGET_LIST "smf_cluster;smf_protocol_hci;smf_http_srv;smf_https_srv;smf_protocol_iec;smf_protocol_imega;smf_info;smf_bus_ipt;smf_protocol_ipt;smf_lora;smf_protocol_mbus;smf_protocol_modem;smf_bus_serial;smf_bus_sml;smf_protocol_sml;smf_protocol_telnet;dash;dashs;broker_iec;broker_mbus_radio;e350;collector;emitter;ipt;ipts;segw;store;lora;master;modem;setup;csv")

if(${PROJECT_NAME}_SSL_SUPPORT)
    list(APPEND "lora" NODE_TARGET_LIST)
    list(APPEND "admin" NODE_TARGET_LIST)
endif()
if(MSVC)
    list(APPEND "mqtt" NODE_TARGET_LIST)
endif(MSVC)

list(LENGTH NODE_TARGET_LIST NODE_TARGET_LIST_SIZE)
message(STATUS "** Target list [${NODE_TARGET_LIST_SIZE}] : ${NODE_TARGET_LIST}")

install(TARGETS ${NODE_TARGET_LIST}
    EXPORT SMFTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

#
# Packaging
# see https://cliutils.gitlab.io/modern-cmake/chapters/install/installing.html
#
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/node/ConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY AnyNewerVersion
)

export(EXPORT SMFTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/node/NodeTargets.cmake"
  NAMESPACE SMF::
)
configure_file(cmake/NodeConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/node/NodeConfig.cmake"
    COPYONLY
)

set(ConfigPackageLocation lib/cmake/node)

# install(EXPORT SMFTargets
#   FILE
#     NodeTargets.cmake
#   NAMESPACE
#     Upstream::
#   DESTINATION
#     ${ConfigPackageLocation}
# )

install(
  FILES
    cmake/NodeConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/node/ConfigVersion.cmake"
  DESTINATION
    ${ConfigPackageLocation}
  COMPONENT
    Devel
)

#
# CPack configuration
#
set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_NAME} - Smart Metering Framework")
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION_TWEAK ${PROJECT_VERSION_TWEAK})
set(CPACK_PACKAGE_CONTACT "info@solostec.ch")
set(CPACK_PACKAGE_VENDOR "solosTec")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_RPM_PACKAGE_DESCRIPTION "SMF library")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/logo.ico)

if(UNIX)
#	set(CPACK_PACKAGING_INSTALL_PREFIX "/opt")
	set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
	set(CPACK_GENERATOR "RPM")
elseif(WIN32)

	set(CPACK_GENERATOR "NSIS64")
	set(CPACK_NSIS_MUI_ICON ${PROJECT_SOURCE_DIR}/logo.ico)
	set(CPACK_NSIS_MUI_UNIICON ${PROJECT_SOURCE_DIR}/logo.ico)
	#set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\vcredist_x86.exe\\\" /q:a'")
	set(CPACK_NSIS_MODIFY_PATH ON)
	set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.solostec.com")

else()
	set(CPACK_GENERATOR "DEB")
endif()


include(CPack)
