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
math(EXPR PATCH_LEVEL "(${THIS_YEAR} * 1000) + ${PATCH_LEVEL}")

#
# set project name/properties
#
project(NODE 
	VERSION 0.7.${PATCH_LEVEL}.1
	LANGUAGES CXX C)


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
string(TIMESTAMP NOW_UTC "%Y-%m-%dT%H:%M:%SZ")
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
message(STATUS "** Compiler        : ${CMAKE_CXX_COMPILER_ID} v${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "** Timestamp       : ${${PROJECT_NAME}_NOW_UTC}")
message(STATUS "** CPU Cores       : ${${PROJECT_NAME}_CPU_COUNT}")
message(STATUS "** Pool Size       : ${${PROJECT_NAME}_POOL_SIZE}")
message(STATUS "** Patchlevel      : ${PROJECT_VERSION_PATCH}")
if(UNIX)
# has no meaning on VS
# set default cmake build type to RelWithDebInfo (None Debug Release RelWithDebInfo MinSizeRel)
if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE "RelWithDebInfo" )
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
#	setup C++ compiler
#
if (CMAKE_COMPILER_IS_GNUCXX)

	#
	# gnu C++
	#
	
	# -std=c++98
	# C++11 since 4.8.1
	# C++14 since 6.1
	# C++17 since 7.0 (?)
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS  "4.8.1")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98") 
		set(${PROJECT_NAME}_LEGACY_MODE ${PROJECT_NAME}_LEGACY_MODE_ON)
		message(STATUS "** C++ support     : C++98")
	elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS  "5.5")
	# crosscompile OECP with gcc 5.4.0 
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11") 
		set(${PROJECT_NAME}_LEGACY_MODE ${PROJECT_NAME}_LEGACY_MODE_ON)
		message(STATUS "** C++ support     : C++11")
	elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS  "6.1")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11") 
		set(${PROJECT_NAME}_LEGACY_MODE ${PROJECT_NAME}_LEGACY_MODE_ON)
		message(STATUS "** C++ support     : C++11")
	elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS  "7.0")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17") 
		set(${PROJECT_NAME}_LEGACY_MODE ${PROJECT_NAME}_LEGACY_MODE_OFF)
		message(STATUS "** C++ support     : C++17")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17") 
		set(${PROJECT_NAME}_LEGACY_MODE ${PROJECT_NAME}_LEGACY_MODE_OFF)
		message(STATUS "** C++ support     : C++17")
	endif()
	
	# only shared libraries
	set(GLOBAL_LIBRARY_TYPE SHARED)
	
elseif(MSVC)

	#
	# Microsoft C++
	#
	
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "18.0")
	# 	prior Visual Studio 2013
		message(FATAL_ERROR "Insufficient MSVC version")
	elseif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "19.12")
	# Visual Studio 2017 15.6.0
		set(${PROJECT_NAME}_LEGACY_MODE ${PROJECT_NAME}_LEGACY_MODE_OFF)
	elseif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "19.0")
	#	after Visual Studio 2015
		set(${PROJECT_NAME}_LEGACY_MODE ${PROJECT_NAME}_LEGACY_MODE_ON)
	else()
		set(${PROJECT_NAME}_LEGACY_MODE ${PROJECT_NAME}_LEGACY_MODE_ON)
	endif()

	# only static libraries
	set(GLOBAL_LIBRARY_TYPE STATIC)

	#	This is Windows 7 (and rules out windows vista)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /bigobj /std:c++latest /MP")	
	add_definitions(-D_WIN32_WINNT=0x0601 -D_SCL_SECURE_NO_WARNINGS -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
	
	
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
#	setup Boost library
#

#
# BOOST_VER is "1_67", "1_68" or "1_69"
#
function(windows_boost_fix BOOST_VER)
  set(BOOST_ROOT "C:/local/boost_${BOOST_VER}_0" CACHE PATH "BOOST_ROOT")
  set(BOOST_LIBRARYDIR "C:/local/boost_${BOOST_VER}_0/lib64-msvc-14.1" CACHE PATH "BOOST_LIBRARYDIR")
  set(BOOST_INCLUDE_DIR "C:/local/boost_${BOOST_VER}_0" CACHE PATH "BOOST_ROOT")
  set(BOOST_DIR "C:/local/boost_${BOOST_VER}" CACHE PATH "BOOST_ROOT")
  message(STATUS "** Search Boost    : overwrite with ${BOOST_ROOT}")
  set(Boost_ATOMIC_LIBRARY_DEBUG "${BOOST_LIBRARYDIR}/libboost_atomic-vc141-mt-gd-x64-${BOOST_VER}.lib" PARENT_SCOPE)
  set(Boost_DATE_TIME_LIBRARY_DEBUG "${BOOST_LIBRARYDIR}/libboost_date_time-vc141-mt-gd-x64-${BOOST_VER}.lib" PARENT_SCOPE)
  set(Boost_FILESYSTEM_LIBRARY_DEBUG "${BOOST_LIBRARYDIR}/libboost_filesystem-vc141-mt-gd-x64-${BOOST_VER}.lib" PARENT_SCOPE)
  set(Boost_PROGRAM_OPTIONS_LIBRARY_DEBUG "${BOOST_LIBRARYDIR}/libboost_program_options-vc141-mt-gd-x64-${BOOST_VER}.lib" PARENT_SCOPE)
  set(Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE "${BOOST_LIBRARYDIR}/boost_program_options-vc141-mt-x64-${BOOST_VER}.lib" PARENT_SCOPE)
  set(Boost_RANDOM_LIBRARY_DEBUG "${BOOST_LIBRARYDIR}/libboost_random-vc141-mt-gd-x64-${BOOST_VER}.lib" PARENT_SCOPE)
  set(Boost_REGEX_LIBRARY_DEBUG "${BOOST_LIBRARYDIR}/libboost_regex-vc141-mt-gd-x64-${BOOST_VER}.lib" PARENT_SCOPE)
  set(Boost_SYSTEM_LIBRARY_DEBUG "${BOOST_LIBRARYDIR}/libboost_system-vc141-mt-gd-x64-${BOOST_VER}.lib" PARENT_SCOPE)
  set(Boost_SYSTEM_LIBRARY_RELEASE "${BOOST_LIBRARYDIR}/libboost_system-vc141-mt-x64-${BOOST_VER}.lib" PARENT_SCOPE)
  set(Boost_THREAD_LIBRARY_DEBUG "${BOOST_LIBRARYDIR}/libboost_thread-vc141-mt-gd-x64-${BOOST_VER}.lib" PARENT_SCOPE)
  set(Boost_UNIT_TEST_FRAMEWORK_LIBRARY_DEBUG "${BOOST_LIBRARYDIR}/libboost_unit_test_framework-vc141-mt-gd-x64-${BOOST_VER}.lib" PARENT_SCOPE)

endfunction()

if(NOT ${PROJECT_NAME}_CROSS_COMPILE)
    if(UNIX)
        if(EXISTS "$ENV{HOME}/projects/boost_1_69_0")
            set(BOOST_ROOT "$ENV{HOME}/projects/boost_1_69_0" CACHE PATH "BOOST_ROOT")
            set(BOOST_LIBRARYDIR "$ENV{HOME}/projects/boost_1_69_0/lib" CACHE PATH "BOOST_LIBRARYDIR")
            message(STATUS "** Search Boost    : overwrite with ${BOOST_ROOT}")
        elseif(EXISTS "$ENV{HOME}/projects/boost_1_68_0/install")
            set(BOOST_ROOT "$ENV{HOME}/projects/boost_1_68_0/install" CACHE PATH "BOOST_ROOT")
            set(BOOST_LIBRARYDIR "$ENV{HOME}/projects/boost_1_68_0/install/lib" CACHE PATH "BOOST_LIBRARYDIR")
            message(STATUS "** Search Boost    : overwrite with ${BOOST_ROOT}")
        elseif(EXISTS "$ENV{HOME}/projects/boost_1_67_0/install")
            set(BOOST_ROOT "$ENV{HOME}/projects/boost_1_67_0/install" CACHE PATH "BOOST_ROOT")
            set(BOOST_LIBRARYDIR "$ENV{HOME}/projects/boost_1_67_0/install/lib" CACHE PATH "BOOST_LIBRARYDIR")
            message(STATUS "** Search Boost    : overwrite with ${BOOST_ROOT}")
        elseif(EXISTS "$ENV{HOME}/projects/boost_1_66_0/install")
            set(BOOST_ROOT "$ENV{HOME}/projects/boost_1_66_0/install" CACHE PATH "BOOST_ROOT")
            set(BOOST_LIBRARYDIR "$ENV{HOME}/projects/boost_1_66_0/install/lib" CACHE PATH "BOOST_LIBRARYDIR")
            message(STATUS "** Search Boost    : overwrite with ${BOOST_ROOT}")
        endif()
    elseif(WIN32)
        if(EXISTS "C:/local/boost_1_69_0")
            windows_boost_fix("1_69")
        elseif(EXISTS "C:/local/boost_1_68_0")
            windows_boost_fix("1_68")
        elseif(EXISTS "C:/local/boost_1_67_0")
            windows_boost_fix("1_67")
        endif()
    endif(UNIX)
endif()

set(Boost_ADDITIONAL_VERSIONS "1.65.0" "1.66.0" "1.67.0" "1.68.0" "1.69.0")
message(STATUS "** Search Boost    : ${Boost_ADDITIONAL_VERSIONS}")
find_package(Boost 1.66 REQUIRED COMPONENTS thread system filesystem program_options random unit_test_framework)

if(Boost_FOUND)

	message(STATUS "** Boost Version    : ${Boost_VERSION}")
	message(STATUS "** Boost Include    : ${Boost_INCLUDE_DIRS}")
	message(STATUS "** Boost Path       : ${Boost_LIBRARY_DIRS}")
	message(STATUS "** Boost Libraries  : ${Boost_LIBRARIES}")

	if(Boost_VERSION VERSION_LESS "1.66.0")
		#
		# When working with a Boost version prior 1.66.0 newer versions of Asio and Beast 
		# are required.
		#
		message(WARING "** Newer versions of Asio and Beast are required")

		if(UNIX)
			include_directories($ENV{HOME}/projects/beast/include)
			include_directories($ENV{HOME}/projects/asio/include)
		endif()
	endif()

	include_directories(${Boost_INCLUDE_DIRS})
	link_directories(${Boost_LIBRARY_DIRS})
	
        set(${PROJECT_NAME}_BOOST_FOUND ${PROJECT_NAME}_BOOST_LIBRARY_FOUND)
        set(${PROJECT_NAME}_BOOST_VERSION ${Boost_VERSION})
	
	# increase MPL list size for Boost.Variant
	# possible values are  30, 40 or 50 
	#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
	#define BOOST_MPL_LIMIT_LIST_SIZE 30
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_MPL_CFG_NO_PREPROCESSED_HEADERS -DBOOST_MPL_LIMIT_LIST_SIZE=50 -DBOOST_ASIO_ENABLE_HANDLER_TRACKING")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_MPL_CFG_NO_PREPROCESSED_HEADERS -DBOOST_MPL_LIMIT_LIST_SIZE=50 -DFUSION_MAX_VECTOR_SIZE=50")

else()
	set(${PROJECT_NAME}_BOOST_LIBRARY ${PROJECT_NAME}_BOOST_LIBRARY_NOT_FOUND)
endif(Boost_FOUND)

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
    
        include_directories(${OPENSSL_INCLUDE_DIR})
        link_directories(${OPENSSL_LIBRARIES})
            set(${PROJECT_NAME}_SSL_VERSION ${OPENSSL_VERSION})
    #
    else()
            set(${PROJECT_NAME}_SSL_VERSION "unknown")
    endif()
    
else()

    message(WARNING "** no SSL support")
    
endif()

#
#	setup cyng library
#   -DCYNG_ROOT:path=...
#	assume parallel installation - that is both projects share the same parent directory.
#
if(NOT CYNG_ROOT)
    set(CYNG_ROOT "${PROJECT_SOURCE_DIR}/../cyng" CACHE PATH "CYNG_ROOT")
	message(STATUS "** Set CYNG_ROOT: ${CYNG_ROOT}")
endif()

get_filename_component(CYNG_INCLUDE_DIR "${CYNG_ROOT}/src/main/include" REALPATH)
include_directories("${CYNG_INCLUDE_DIR}")
message(STATUS "** CYNG include path : ${CYNG_INCLUDE_DIR}")

#
#	setup cyng build directory
#   -DCYNG_BUILD:path=...
#	assume parallel installation - that is both projects share the same parent directory.
#
if (NOT CYNG_BUILD)
    set(CYNG_BUILD "${CYNG_ROOT}/build" CACHE PATH "CYNG_BUILD")
	message(STATUS "** Set CYNG_BUILD: ${CYNG_BUILD}")
endif()

get_filename_component(CYNG_LIBRARY_DIR ${CYNG_BUILD} REALPATH)
include_directories(${CYNG_LIBRARY_DIR})
message(STATUS "** CYNG include path : ${CYNG_LIBRARY_DIR}")

if (UNIX)
    link_directories(${CYNG_LIBRARY_DIR})
    message(STATUS "** CYNG Libraries    : ${CYNG_LIBRARY_DIR}")
else()
	#
	#	$(ConfigurationName) is a variable used by the MS build system
	#
	link_directories(${CYNG_LIBRARY_DIR}/$(ConfigurationName))
    message(STATUS "** CYNG Libraries    : ${CYNG_LIBRARY_DIR}")
endif()

#
#	Use CMake modules from CYNG project
#
get_filename_component(cyng_MODULE_PATH "${CYNG_ROOT}/src/modules/" REALPATH)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${cyng_MODULE_PATH}")
message(STATUS "** CMake modules     : ${CMAKE_MODULE_PATH}")

#
# libpugixml-dev (v1.8)
# On Windows set CMake variables PugiXML_INCLUDE_DIRS and PugiXML_LIBRARIES 
#
# Included as 3party software
#
set(PUGIXML_INCLUDE_DIR "${CYNG_ROOT}/3party/pugixml-190")
message(STATUS "** PugiXML Include   : ${PUGIXML_INCLUDE_DIR}")
add_definitions(-D${PROJECT_NAME}_PUGIXML_INSTALLED)
include_directories(${PUGIXML_INCLUDE_DIR})

#
# include directories for all C/C++ projects
#
include_directories(src/main/include)

#
# configure a header file to pass some of the CMake settings
# to the source code
#
configure_file (
  "${PROJECT_SOURCE_DIR}/src/main/templates/project_info.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h"
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

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
#	cluster library (client side)
#
include (lib/cluster/lib.cmake)
add_library(smf_cluster ${GLOBAL_LIBRARY_TYPE} ${cluster_lib})
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	target_link_libraries(smf_cluster cyng_domain)
endif()

#
#	ipt protocol library
#
include (lib/ipt/protocol/lib.cmake)
add_library(smf_protocol_ipt ${GLOBAL_LIBRARY_TYPE} ${ipt_protocol_lib})
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	target_link_libraries(smf_protocol_ipt cyng_vm)
endif()

#
#	ipt bus library (client)
#
include (lib/ipt/bus/lib.cmake)
add_library(smf_bus_ipt ${GLOBAL_LIBRARY_TYPE} ${ipt_bus_lib})
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	target_link_libraries(smf_bus_ipt cyng_domain)
endif()

#
#	sml protocol library
#
include (lib/sml/protocol/lib.cmake)
add_library(smf_protocol_sml ${GLOBAL_LIBRARY_TYPE} ${sml_protocol_lib})
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	target_link_libraries(smf_protocol_sml cyng_vm cyng_sys)
else()
	target_link_libraries(smf_protocol_sml cyng_table cyng_store)
endif()


#
#	sml bus library (client)
#
include (lib/sml/bus/lib.cmake)
add_library(smf_bus_sml ${GLOBAL_LIBRARY_TYPE} ${sml_bus_lib})
set(sml_bus_link_libs smf_protocol_sml)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND sml_bus_link_libs cyng_domain)
endif()
target_link_libraries(smf_bus_sml ${sml_bus_link_libs})

#
#	modem/AT protocol library
#
include (lib/modem/protocol/lib.cmake)
add_library(smf_protocol_modem ${GLOBAL_LIBRARY_TYPE} ${modem_protocol_lib})
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	target_link_libraries(smf_protocol_modem cyng_vm)
endif()

#
#	modem/AT bus library (client)
#
#include (lib/modem/bus/lib.cmake)
#add_library(smf_bus_modem ${GLOBAL_LIBRARY_TYPE} ${modem_bus_lib})
#if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
#	target_link_libraries(smf_bus_modem cyng_domain)
#endif()

#
#	M-Bus/EN 13757-3 protocol/application layer library
#
include (lib/mbus/protocol/lib.cmake)
add_library(smf_protocol_mbus ${GLOBAL_LIBRARY_TYPE} ${mbus_protocol_lib})
#if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
#	target_link_libraries(smf_protocol_mbus cyng_vm)
#endif()

#
#	iMega/CU protocol library
#
include (lib/imega/protocol/lib.cmake)
add_library(smf_protocol_imega ${GLOBAL_LIBRARY_TYPE} ${imega_protocol_lib})
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	target_link_libraries(smf_protocol_imega cyng_vm)
endif()

#
#	IEC 62056-21 protocol library
#
include (lib/iec/protocol/lib.cmake)
add_library(smf_protocol_iec ${GLOBAL_LIBRARY_TYPE} ${iec_protocol_lib})

#
#	LoRa payload library
#
include (lib/lora/payload/lib.cmake)
add_library(smf_lora ${GLOBAL_LIBRARY_TYPE} ${lora_payload_lib})

#
#	http server library
#
include (lib/http/server/lib.cmake)
add_library(smf_http_srv ${GLOBAL_LIBRARY_TYPE} ${http_srv_lib})
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	if(${PROJECT_NAME}_SSL_SUPPORT)
		target_link_libraries(smf_http_srv cyng_vm cyng_crypto smf_cluster)
	else()
		target_link_libraries(smf_http_srv cyng_vm smf_cluster)
	endif()
endif()

#
# https server library
#
if(${PROJECT_NAME}_SSL_SUPPORT)
	include (lib/https/server/lib.cmake)
	add_library(smf_https_srv ${GLOBAL_LIBRARY_TYPE} ${https_srv_lib})
	if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	   target_link_libraries(smf_https_srv cyng_crypto cyng_io cyng_vm smf_cluster)
	else()
		target_link_libraries(smf_https_srv cyng_crypto smf_cluster)
	endif()
endif()

if(${PROJECT_NAME}_BUILD_TEST AND ${PROJECT_NAME}_SSL_SUPPORT)
#
# This program is only for testing puposes
# generic HTTP server
#
	include (nodes/http/prg.cmake)
	add_executable(http ${node_http})
	# libraries to link 
	set(http_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_mail cyng_crypto cyng_sys ${OPENSSL_LIBRARIES} smf_http_srv)
	if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
		list(APPEND http_link_libs "${Boost_LIBRARIES}")
		if (UNIX)
			list(APPEND http_link_libs pthread)
		endif()
	endif()
	target_link_libraries(http ${http_link_libs})
endif()

if(${PROJECT_NAME}_BUILD_TEST AND ${PROJECT_NAME}_SSL_SUPPORT)
#
# This program is only for testing puposes
# generic HTTPS server
#
	include (nodes/https/prg.cmake)
	add_executable(https ${node_https})
	# libraries to link 
	set(https_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_crypto cyng_sys cyng_vm smf_https_srv ${OPENSSL_LIBRARIES})
	if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
		list(APPEND https_link_libs "${Boost_LIBRARIES}")
		if (UNIX)
			list(APPEND https_link_libs pthread)
		endif()
	endif()
	target_link_libraries(https ${https_link_libs})
endif()


if(${PROJECT_NAME}_BUILD_TEST)
#
# This program is only for testing purposes
# https://raw.githubusercontent.com/boostorg/beast/develop/example/advanced/server/advanced_server.cpp
#
	include (nodes/as/prg.cmake)
	add_executable(as ${node_as})
	if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
		if (UNIX)
			target_link_libraries(as ${Boost_LIBRARIES} pthread)
		else()
			target_link_libraries(as ${Boost_LIBRARIES})
		endif()
	endif()
endif()

if(${PROJECT_NAME}_BUILD_TEST AND ${PROJECT_NAME}_SSL_SUPPORT)
#
# This program is only for testing purposes
	include (nodes/assl/prg.cmake)
	add_executable(assl ${node_assl})
	target_link_libraries(assl ${OPENSSL_LIBRARIES})
	if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
		if (UNIX)
			target_link_libraries(assl ${Boost_LIBRARIES} pthread)
		else()
			target_link_libraries(assl ${Boost_LIBRARIES})
		endif()
	endif()
endif()

#
# dashboard (HTTP)
#
if(${PROJECT_NAME}_SSL_SUPPORT)
    include (nodes/dash/prg.cmake)
    add_executable(dash ${node_dash})
    # libraries to link
    set(dashboard_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_store cyng_table cyng_sys cyng_xml smf_cluster smf_http_srv ${OPENSSL_LIBRARIES})

	message(STATUS "** dash auth support : ${${PROJECT_NAME}_SSL_SUPPORT}")
	if(${PROJECT_NAME}_SSL_SUPPORT)
		list(APPEND dashboard_link_libs cyng_crypto)
	endif()

    if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
        list(APPEND dashboard_link_libs "${Boost_LIBRARIES}")
        if (UNIX)
            list(APPEND dashboard_link_libs pthread)
        endif()
    endif()
    target_link_libraries(dash ${dashboard_link_libs})
endif()

#
# dashboard (HTTPS)
#
if(${PROJECT_NAME}_SSL_SUPPORT)
    include (nodes/dashs/prg.cmake)
    add_executable(dashs ${node_dashs})
    # libraries to link
    set(dashsboard_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_store cyng_table cyng_sys cyng_xml smf_cluster smf_https_srv ${OPENSSL_LIBRARIES})
    if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
        list(APPEND dashsboard_link_libs "${Boost_LIBRARIES}")
        if (UNIX)
            list(APPEND dashsboard_link_libs pthread)
        endif()
    endif()
    target_link_libraries(dashs ${dashsboard_link_libs})
endif()

#
# e350
#
include (nodes/e350/prg.cmake)
add_executable(e350 ${node_e350})
# libraries to link
set(e350_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_sys cyng_rnd smf_cluster smf_protocol_imega)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND e350_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND e350_link_libs pthread)
    endif()
endif()
target_link_libraries(e350 ${e350_link_libs})

#
# IP-T collector
#
include (nodes/ipt/collector/prg.cmake)
add_executable(collector ${node_ipt_collector})
# libraries to link
set(collector_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_table cyng_sys smf_protocol_ipt smf_bus_ipt)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND collector_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND collector_link_libs pthread)
    endif()
endif()
target_link_libraries(collector ${collector_link_libs})

#
# IP-T emitter
#
include (nodes/ipt/emitter/prg.cmake)
add_executable(emitter ${node_ipt_emitter})
# libraries to link
set(emitter_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_table cyng_sys cyng_crypto smf_protocol_ipt smf_bus_ipt)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND emitter_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND emitter_link_libs pthread)
    endif()
endif()
target_link_libraries(emitter ${emitter_link_libs})

#
# IP-T gateway
#
include (nodes/ipt/gateway/prg.cmake)
add_executable(gateway ${node_ipt_gateway})
# libraries to link
set(gateway_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_store cyng_table cyng_domain cyng_sys cyng_rnd smf_protocol_ipt smf_bus_ipt smf_bus_sml)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND gateway_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND gateway_link_libs pthread)
    endif()
endif()
if(WIN32)
	list(APPEND gateway_link_libs odbc32.lib cyng_sqlite3)
else()
	list(APPEND gateway_link_libs ${SQLite3_LIBRARY})
endif(WIN32)

target_link_libraries(gateway ${gateway_link_libs})

#
# IP-T master
#
include (nodes/ipt/master/prg.cmake)
add_executable(ipt ${node_ipt_master})
# libraries to link
set(ipt_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_sys cyng_rnd smf_cluster smf_protocol_ipt smf_protocol_sml)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND ipt_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND ipt_link_libs pthread)
    endif()
endif()
#message(STATUS "** link ipt_master       : ${ipt_link_libs}")
target_link_libraries(ipt ${ipt_link_libs})

#
# IP-T store
#
include (nodes/ipt/store/prg.cmake)
add_executable(store ${node_ipt_store})
# libraries to link
set(store_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_db cyng_sql cyng_store cyng_table cyng_xml cyng_sys smf_protocol_ipt smf_bus_ipt smf_protocol_sml smf_protocol_iec)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND store_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND store_link_libs pthread ${CMAKE_DL_LIBS})
    endif()
endif()
if(WIN32)
	list(APPEND store_link_libs odbc32.lib cyng_sqlite3)
else()
	list(APPEND store_link_libs ${SQLite3_LIBRARY})
endif(WIN32)
target_link_libraries(store ${store_link_libs})
	
if(${PROJECT_NAME}_BUILD_TEST)
#
# This program is only for testing purposes
# IP-T stress
#
	include (nodes/ipt/stress/prg.cmake)
	add_executable(stress ${node_ipt_stress})
	# libraries to link
	set(stress_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_db cyng_sql cyng_store cyng_table cyng_xml cyng_sys smf_protocol_ipt smf_bus_ipt smf_protocol_sml)
	if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
		list(APPEND stress_link_libs "${Boost_LIBRARIES}")
		if (UNIX)
			list(APPEND stress_link_libs pthread)
		endif()
	endif()
	if(WIN32)
		list(APPEND stress_link_libs odbc32.lib cyng_sqlite3)
	else()
		list(APPEND stress_link_libs ${SQLite3_LIBRARY})
	endif(WIN32)
	target_link_libraries(stress ${stress_link_libs})
endif()

#
# LoRa
#
if(${PROJECT_NAME}_SSL_SUPPORT)
    include (nodes/lora/prg.cmake)
    add_executable(lora ${node_lora})
    # libraries to link
    set(lora_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_sys cyng_crypto cyng_xml smf_cluster smf_https_srv smf_lora smf_protocol_sml ${OPENSSL_LIBRARIES})
    if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
        list(APPEND lora_link_libs "${Boost_LIBRARIES}")
        if (UNIX)
            list(APPEND lora_link_libs pthread)
        endif()
    endif()
    target_link_libraries(lora ${lora_link_libs})
endif()

#
# master
#
include (nodes/master/prg.cmake)
add_executable(master ${node_master})
# libraries to link
set(master_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_store cyng_table cyng_vm cyng_domain cyng_sys)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND master_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND master_link_libs pthread)
    endif()
endif()
target_link_libraries(master ${master_link_libs})

#
# modem
#
include (nodes/modem/prg.cmake)
add_executable(modem ${node_modem})
# libraries to link
set(modem_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_sys smf_cluster smf_protocol_modem)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND modem_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND modem_link_libs pthread)
    endif()
endif()
#message(STATUS "** link modem            : ${modem_link_libs}")
target_link_libraries(modem ${modem_link_libs})

#
# mqtt
#
include (nodes/mqtt/prg.cmake)
add_executable(mqtt ${node_mqtt})
target_compile_options(mqtt PRIVATE "-DMQTT_NO_TLS")
target_include_directories(mqtt PRIVATE "${PROJECT_SOURCE_DIR}/3party")
# libraries to link
set(mqtt_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_sys cyng_domain cyng_table smf_cluster)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND mqtt_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND mqtt_link_libs pthread)
    endif()
endif()
target_link_libraries(mqtt ${mqtt_link_libs})

#
# iec_62056
#
include (nodes/iec-62056/prg.cmake)
add_executable(iec_62056 ${node_iec_62056})
# libraries to link
set(iec_62056_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_sys cyng_domain cyng_table smf_cluster)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND iec_62056_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND iec_62056_link_libs pthread)
    endif()
endif()
target_link_libraries(iec_62056 ${iec_62056_link_libs})

#
# setup
#
include (nodes/setup/prg.cmake)
add_executable(setup ${node_setup})
# libraries to link
set(setup_link_libs cyng_core cyng_io cyng_async cyng_log cyng_parser cyng_json cyng_vm cyng_domain cyng_db cyng_sql cyng_store cyng_table cyng_sys smf_cluster)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND setup_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND setup_link_libs pthread ${CMAKE_DL_LIBS})
    endif()
endif()
if(WIN32)
	list(APPEND setup_link_libs odbc32.lib cyng_sqlite3)
else()
	list(APPEND setup_link_libs ${SQLite3_LIBRARY})
endif(WIN32)
target_link_libraries(setup ${setup_link_libs})

#
# task: csv
#
include (tasks/csv/prg.cmake)
add_executable(csv ${task_csv})

# libraries to link
set(csv_link_libs cyng_core cyng_io cyng_async cyng_log cyng_parser cyng_json cyng_vm cyng_domain cyng_db cyng_sql cyng_store cyng_table cyng_sys smf_protocol_sml smf_cluster)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND csv_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND csv_link_libs pthread ${CMAKE_DL_LIBS})
    endif()
endif()
if(WIN32)
	list(APPEND csv_link_libs odbc32.lib cyng_sqlite3)
else()
	list(APPEND csv_link_libs ${SQLite3_LIBRARY})
endif(WIN32)
target_link_libraries(csv ${csv_link_libs})

#
# test unit using Boost.Test
# BOOST_TEST_DYN_LINK is required to build a main() function
# cmake -DNODE_BUILD_TEST:bool=ON ..
#

if(${PROJECT_NAME}_BUILD_TEST AND ${PROJECT_NAME}_SSL_SUPPORT)
	include (test/unit-test/unit-test.cmake)
	add_executable(unit_test ${unit_test})
		
	# BOOST_TEST_DYN_LINK is required to build a main() function
	set_property(
		TARGET unit_test
		PROPERTY COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK BOOST_ASIO_HAS_MOVE)

	set(unittest_link_libs cyng_core cyng_io cyng_async cyng_log cyng_store cyng_vm cyng_domain cyng_sql cyng_json cyng_parser cyng_mail cyng_crypto cyng_sys cyng_db cyng_table cyng_xml smf_protocol_ipt smf_bus_ipt smf_protocol_sml smf_protocol_mbus)

	if (${PROJECT_NAME}_PUGIXML_INSTALLED)
		list(APPEND unittest_link_libs cyng_xml)
	endif()

	if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
		list(APPEND unittest_link_libs ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})
	else()
		if(WIN32)
			list(APPEND unittest_link_libs odbc32.lib cyng_sqlite3)
		else()
			list(APPEND unittest_link_libs pthread ${CMAKE_DL_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${SQLite3_LIBRARY})
		endif()
	endif()

	message(STATUS "** link unit-test        : ${unittest_link_libs}")
	target_link_libraries(unit_test ${unittest_link_libs})
endif()


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

	# http
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/http/templates/http.linux.cgf.in"
		  "${PROJECT_BINARY_DIR}/http_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/http/templates/http.service.in"
		  "${PROJECT_BINARY_DIR}/node-http.service")

	# dash
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dash/templates/dash.linux.cgf.in"
		  "${PROJECT_BINARY_DIR}/dash_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dash/templates/dash.service.in"
		  "${PROJECT_BINARY_DIR}/node-dash.service")

	# dashs
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dashs/templates/dashs.linux.cgf.in"
		  "${PROJECT_BINARY_DIR}/dashs_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dashs/templates/dashs.service.in"
		  "${PROJECT_BINARY_DIR}/node-dashs.service")

	# e355
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350.linux.cgf.in"
		  "${PROJECT_BINARY_DIR}/e350_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350.service.in"
		  "${PROJECT_BINARY_DIR}/node-e350.service")

	# ipt collector 
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector.linux.cgf.in"
		  "${PROJECT_BINARY_DIR}/collector_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector.service.in"
		  "${PROJECT_BINARY_DIR}/node-collector.service")

	# ipt emitter 
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter.linux.cgf.in"
		  "${PROJECT_BINARY_DIR}/emitter_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter.service.in"
		  "${PROJECT_BINARY_DIR}/node-emitter.service")
	  
	# ipt gateway
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway.linux.cgf.in"
		  "${PROJECT_BINARY_DIR}/gateway_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway.service.in"
		  "${PROJECT_BINARY_DIR}/node-gateway.service")

	# ipt master
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt.linux.cgf.in"
		  "${PROJECT_BINARY_DIR}/ipt_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt.service.in"
		  "${PROJECT_BINARY_DIR}/node-ipt.service")

	# ipt store
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store.linux.cgf.in"
		  "${PROJECT_BINARY_DIR}/store_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store.service.in"
		  "${PROJECT_BINARY_DIR}/node-store.service")

	# lora
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora.linux.cgf.in"
		  "${PROJECT_BINARY_DIR}/lora_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora.service.in"
		  "${PROJECT_BINARY_DIR}/node-lora.service")
	  
	# master
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/master/templates/master.linux.cgf.in"
		  "${PROJECT_BINARY_DIR}/master_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/master/templates/master.service.in"
		  "${PROJECT_BINARY_DIR}/node-master.service")

	# modem
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem.linux.cgf.in"
		  "${PROJECT_BINARY_DIR}/modem_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem.service.in"
		  "${PROJECT_BINARY_DIR}/node-modem.service")

	# mqtt
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt.linux.cgf.in"
		  "${PROJECT_BINARY_DIR}/mqtt_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt.service.in"
		  "${PROJECT_BINARY_DIR}/node-mqtt.service")


	# setup
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup.linux.cgf.in"
		  "${PROJECT_BINARY_DIR}/setup_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup.service.in"
		  "${PROJECT_BINARY_DIR}/node-setup.service")
	  
      # csv
      configure_file (
            "${PROJECT_SOURCE_DIR}/tasks/csv/templates/csv.linux.cgf.in"
            "${PROJECT_BINARY_DIR}/csv_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

      configure_file (
            "${PROJECT_SOURCE_DIR}/tasks/csv/templates/csv.service.in"
            "${PROJECT_BINARY_DIR}/task-csv.service")


else()


	# http
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/http/templates/http.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/http_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/http/templates/http_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/http_create_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/http/templates/http_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/http_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/http/templates/http_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/http_restart_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/http/templates/http.rc.in"
		  "${PROJECT_BINARY_DIR}/http.rc")

	# https
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/https/templates/https.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/https_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/https/templates/https_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/https_create_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/https/templates/https_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/https_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/https/templates/https_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/https_restart_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/https/templates/https.rc.in"
		  "${PROJECT_BINARY_DIR}/https.rc")

	# dash
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dash/templates/dash.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/dash_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dash/templates/dash_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/dash_create_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dash/templates/dash_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/dash_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dash/templates/dash_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/dash_restart_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dash/templates/dash.rc.in"
		  "${PROJECT_BINARY_DIR}/dash.rc")

	# dashs
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dashs/templates/dashs.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/dashs_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dashs/templates/dashs_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/dashs_create_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dashs/templates/dashs_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/dashs_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dashs/templates/dashs_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/dashs_restart_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dashs/templates/dashs.rc.in"
		  "${PROJECT_BINARY_DIR}/dashs.rc")

	# e355
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/e350_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/e350_create_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/e350_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/e350_restart_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350.rc.in"
		  "${PROJECT_BINARY_DIR}/e350.rc")

	# ipt collector 
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/collector_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/collector_create_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/collector_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/collector_restart_service.cmd")

	# ipt emitter 
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/emitter_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/emitter_create_service.cmd")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/emitter_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/emitter_restart_service.cmd")

	# ipt gateway
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/gateway_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/gateway_create_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/gateway_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/gateway_restart_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway.rc.in"
		  "${PROJECT_BINARY_DIR}/gateway.rc")

	# ipt master
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/ipt_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/ipt_create_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/ipt_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/ipt_restart_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt.rc.in"
		  "${PROJECT_BINARY_DIR}/ipt.rc")

	# ipt store
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/store_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/store_create_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/store_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/store_restart_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store.rc.in"
		  "${PROJECT_BINARY_DIR}/store.rc")

	# lora
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/lora_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/lora_create_service.cmd")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/lora_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/lora_restart_service.cmd")

	# master
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/master/templates/master.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/master_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/master/templates/master_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/master_create_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/master/templates/master_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/master_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/master/templates/master_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/master_restart_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/master/templates/master.rc.in"
		  "${PROJECT_BINARY_DIR}/master.rc")

	# modem
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/modem_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/modem_create_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/modem_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/modem_restart_service.cmd")

	# mqtt
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/mqtt_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/mqtt_create_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/mqtt_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/mqtt_restart_service.cmd")

	# setup
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/setup_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup_create_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/setup_create_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup_delete_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/setup_delete_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup_restart_service.cmd.in"
		  "${PROJECT_BINARY_DIR}/setup_restart_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup.rc.in"
		  "${PROJECT_BINARY_DIR}/setup.rc")

      # csv
      configure_file (
            "${PROJECT_SOURCE_DIR}/tasks/csv/templates/csv.windows.cgf.in"
            "${PROJECT_BINARY_DIR}/csv_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

      configure_file (
            "${PROJECT_SOURCE_DIR}/tasks/csv/templates/csv_create_service.cmd.in"
            "${PROJECT_BINARY_DIR}/csv_create_service.cmd")

      configure_file (
            "${PROJECT_SOURCE_DIR}/tasks/csv/templates/csv_delete_service.cmd.in"
            "${PROJECT_BINARY_DIR}/csv_delete_service.cmd")

      configure_file (
            "${PROJECT_SOURCE_DIR}/tasks/csv/templates/csv_restart_service.cmd.in"
            "${PROJECT_BINARY_DIR}/csv_restart_service.cmd")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/tasks/csv/templates/csv.rc.in"
		  "${PROJECT_BINARY_DIR}/csv.rc")

endif(UNIX)

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
