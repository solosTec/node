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
# set project name
project(NODE)

# Add a version number
set (${PROJECT_NAME}_VERSION_MAJOR 0)
set (${PROJECT_NAME}_VERSION_MINOR 2)

string(TIMESTAMP THIS_YEAR "%Y")
set (${PROJECT_NAME}_COPYRIGHT_YEAR ${THIS_YEAR})

# Patch level as year + day of the year
string(TIMESTAMP PATCH_LEVEL "%j")	# day of the year
math(EXPR ${PROJECT_NAME}_PATCH_LEVEL "(${THIS_YEAR} * 1000) + ${PATCH_LEVEL}")


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
message(STATUS "**   ")                                        


#
# Collect and dump some basic information
#
string(TIMESTAMP NOW_UTC "%Y-%m-%dT%H:%M:%SZ")
set (${PROJECT_NAME}_NOW_UTC ${NOW_UTC})

include(ProcessorCount)
ProcessorCount(${PROJECT_NAME}_CPU_COUNT)

message(STATUS "** CMake           : v${CMAKE_VERSION}")
message(STATUS "** Generator       : ${CMAKE_GENERATOR}")
message(STATUS "** Platform        : ${CMAKE_SYSTEM}")
message(STATUS "** Compiler        : ${CMAKE_CXX_COMPILER_ID} v${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "** Timestamp       : ${${PROJECT_NAME}_NOW_UTC}")
message(STATUS "** CPU Cores       : ${${PROJECT_NAME}_CPU_COUNT}")
message(STATUS "** Patchlevel      : ${${PROJECT_NAME}_PATCH_LEVEL}")
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
#
string(RANDOM LENGTH 9 ALPHABET "1234567890ABCDEF" __RND_VALUE_SALT)
set (${PROJECT_NAME}_SALT_VALUE 	"0x${__RND_VALUE_SALT}")
set (${PROJECT_NAME}_SALT_STRING 	${__RND_VALUE_SALT})

#
#	Generate password
#
string(RANDOM LENGTH 8 ALPHABET "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" __RND_VALUE_PWD)
set (${PROJECT_NAME}_PWD 	"${__RND_VALUE_PWD}")


#
#	setup C++ compiler
#
if (CMAKE_COMPILER_IS_GNUCC)

	#
	# gnu C++
	#
	
	# -std=c++98
	# C++11 since 4.8.1
	# C++14 since 6.1
	# C++17 since 7.0 (?)
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS  "4.8.1")
		add_definitions(-std=c++98)	
		set(${PROJECT_NAME}_LEGACY_MODE ${PROJECT_NAME}_LEGACY_MODE_ON)
		message(STATUS "** C++ support     : C++98")
	elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS  "6.1")
		add_definitions(-std=c++11)	
		set(${PROJECT_NAME}_LEGACY_MODE ${PROJECT_NAME}_LEGACY_MODE_ON)
		message(STATUS "** C++ support     : C++11")
	elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS  "7.0")
		add_definitions(-std=c++14)	
		set(${PROJECT_NAME}_LEGACY_MODE ${PROJECT_NAME}_LEGACY_MODE_OFF)
		message(STATUS "** C++ support     : C++14")
	else()
		add_definitions(-std=c++17)	
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
	elseif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "19.0")
	#	after Visual Studio 2015
		set(${PROJECT_NAME}_LEGACY_MODE ${PROJECT_NAME}_LEGACY_MODE_OFF)
	else()
		set(${PROJECT_NAME}_LEGACY_MODE ${PROJECT_NAME}_LEGACY_MODE_ON)
	endif()

	# only static libraries
	set(GLOBAL_LIBRARY_TYPE STATIC)

	#	This is Windows 7 (and rules out windows vista)
	set(CMAKE_CXX_FLAGS "-D_WIN32_WINNT=0x0601 -D_SCL_SECURE_NO_WARNINGS")	
	add_definitions(/EHsc /bigobj /std:c++latest)
	
else()

	message( FATAL_ERROR "Unknown or missing compiler: ${CMAKE_CXX_COMPILER_ID}" )
	
endif()

#
#	setup Boost library
#
#set(BOOST_LIBRARYDIR "C:\\local\\boost_1_66_0_b1_rc2\\lib64-msvc-14.1")
#set(BOOST_ROOT "C:\\local\\boost_1_66_0_b1_rc2\\boost")

# if(UNIX)
# set(BOOST_ROOT "$ENV{HOME}/projects/boost_1_66_0/release")
# # set(BOOST_LIBRARYDIR "$ENV{HOME}/projects/boost_1_66_0/release/lib")
# endif(UNIX)
set(Boost_ADDITIONAL_VERSIONS "1.61.0" "1.62.0" "1.63.0" "1.64.0" "1.65.1" "1.66.0")
message(STATUS "** Search Boost    : ${Boost_ADDITIONAL_VERSIONS}")
find_package(Boost 1.61 REQUIRED COMPONENTS thread system date_time filesystem program_options random signals unit_test_framework regex)
#find_package(Boost 1.61 REQUIRED COMPONENTS thread system date_time filesystem)

if(Boost_FOUND)

	message(STATUS "** Boost Version    : ${Boost_VERSION}")
	message(STATUS "** Boost Include    : ${Boost_INCLUDE_DIRS}")
	message(STATUS "** Boost Path       : ${Boost_LIBRARY_DIRS}")
	message(STATUS "** Boost Libraries  : ${Boost_LIBRARIES}")

if (UNIX)
	include_directories($ENV{HOME}/projects/beast/include)
	include_directories($ENV{HOME}/projects/asio/include)
else()
#	include_directories("${PROJECT_SOURCE_DIR}/../beast/include")
#	include_directories("${PROJECT_SOURCE_DIR}/../asio/include")
#	include_directories("${PROJECT_SOURCE_DIR}/../winapi/include")
endif(UNIX)

	include_directories(${Boost_INCLUDE_DIRS})
	link_directories(${Boost_LIBRARY_DIRS})
	
	set(${PROJECT_NAME}_BOOST_LIBRARY ${PROJECT_NAME}_BOOST_LIBRARY_FOUND)
	set(${PROJECT_NAME}_BOOST_VERSION ${PROJECT_NAME}_BOOST_${Boost_VERSION})
	
else()
	set(${PROJECT_NAME}_BOOST_LIBRARY ${PROJECT_NAME}_BOOST_LIBRARY_NOT_FOUND)
endif(Boost_FOUND)

#
#	setup SSL library
#
message(STATUS "** Search openSSL   : ${OPENSSL_ROOT_DIR}")
if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
	find_package(OpenSSL 1.0.2 REQUIRED)
else()
	find_package(OpenSSL 1.0.1 REQUIRED)
endif()
if(OPENSSL_FOUND)

#	message(STATUS "** openSSL Found         : ${OPENSSL_FOUND}")
	message(STATUS "** openSSL Version       : ${OPENSSL_VERSION}")
	message(STATUS "** openSSL Include       : ${OPENSSL_INCLUDE_DIR}")
	message(STATUS "** openSSL crypto library: ${OPENSSL_CRYPTO_LIBRARY}")
	message(STATUS "** openSSL SSL library   : ${OPENSSL_SSL_LIBRARY}")
	message(STATUS "** openSSL Libraries     : ${OPENSSL_LIBRARIES}")
 
 	include_directories(${OPENSSL_INCLUDE_DIR})
 	link_directories(${OPENSSL_LIBRARIES})
	
endif()

#
#	setup cyng library
#	assume parallel installation
#
include_directories("${PROJECT_SOURCE_DIR}/../cyng/src/main/include")
if (UNIX)
	if (CROSS_COMPILE)
		get_filename_component(cyng_LIBRARY_DIRS "$ENV{HOME}/development/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/arm-linux-gnueabihf/sysroot/" REALPATH)
		include_directories("${PROJECT_SOURCE_DIR}/../cyng/cross")
		link_directories("${PROJECT_SOURCE_DIR}/../cyng/cross")
	else()
		get_filename_component(cyng_LIBRARY_DIRS "${PROJECT_SOURCE_DIR}/../cyng/build" REALPATH)
		include_directories("${PROJECT_SOURCE_DIR}/../cyng/build")
 	endif(CROSS_COMPILE)
	message(STATUS "** cyng Libraries   : ${cyng_LIBRARY_DIRS}")
	link_directories(${cyng_LIBRARY_DIRS})
else()
# 	link_directories("${PROJECT_BINARY_DIR}/../cyng")
 	get_filename_component(cyng_LIBRARY_DIRS "${PROJECT_BINARY_DIR}/../cyng" REALPATH)
	include_directories("${PROJECT_SOURCE_DIR}/../cyng")
	include_directories(${cyng_LIBRARY_DIRS})
	link_directories(${cyng_LIBRARY_DIRS})
endif()

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
#	cyng meta library - header only
#
# include (lib/meta/lib.cmake)
# add_library(cyng_meta SHARED ${meta_lib})

#
#	cyng core/object library
#
# include (lib/core/lib.cmake)
# add_library(cyng_core SHARED ${core_lib})

#
#	cyng log library
#
# include (lib/log/lib.cmake)
# add_library(cyng_log SHARED ${log_lib})

#
#	cyng I/O library
#
# include (lib/io/lib.cmake)
# add_library(cyng_io SHARED ${io_lib})

#
#	cyng VM library
#
# include (lib/vm/lib.cmake)
# add_library(cyng_vm SHARED ${vm_lib})

#
#	cyng async library
#
# include (lib/async/lib.cmake)
# add_library(cyng_async SHARED ${async_lib})

#
#	cyng store library
#
# include (lib/store/lib.cmake)
# add_library(cyng_store SHARED ${store_lib})

#
#	cyng sql library
#
# include (lib/sql/lib.cmake)
# add_library(cyng_sql SHARED ${sql_lib})
# target_link_libraries(cyng_sql
# 	cyng_store)
	
#
#	cyng crypto library
#
# include (lib/crypto/lib.cmake)
# add_library(cyng_crypto SHARED ${crypto_lib})

	#
# minimal test program
# hello world!
# cross compile with 
# arm-linux-gnueabihf-g++ -O3 -g3 -Wall -fPIC -o "main.o" -c "main.cpp"
# arm-linux-gnueabihf-g++ -o "hello" main.o
#
include (test/hello.cmake)
add_executable(hello ${hello})

#
# generic HTTP server
#
include (nodes/http/prg.cmake)
add_executable(http ${node_http})
# libraries to link 
set(http_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_mail cyng_crypto cyng_sys ${OPENSSL_LIBRARIES})
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND http_link_libs "${Boost_LIBRARIES}")
endif()
target_link_libraries(http ${http_link_libs})
	
#
# dashboard
#
include (nodes/dash/prg.cmake)
add_executable(dash ${node_dash})
# libraries to link
set(dashboard_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND dashboard_link_libs "${Boost_LIBRARIES}")
endif()
target_link_libraries(dash ${dashboard_link_libs})

#
# IP-T gateway
#
include (nodes/ipt/gateway/prg.cmake)
add_executable(gateway ${node_ipt_gateway})
# libraries to link
set(gateway_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND gateway_link_libs "${Boost_LIBRARIES}")
endif()
target_link_libraries(gateway ${gateway_link_libs})

#
# IP-T master
#
include (nodes/ipt/master/prg.cmake)
add_executable(ipt ${node_ipt_master})
# libraries to link
set(ipt_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND ipt_link_libs "${Boost_LIBRARIES}")
endif()
target_link_libraries(ipt ${ipt_link_libs})

#
# IP-T store
#
include (nodes/ipt/store/prg.cmake)
add_executable(store ${node_ipt_store})
# libraries to link
set(store_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND store_link_libs "${Boost_LIBRARIES}")
endif()
target_link_libraries(store ${store_link_libs})
	
#
# lora
#
include (nodes/lora/prg.cmake)
add_executable(lora ${node_lora})
# libraries to link
set(lora_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND lora_link_libs "${Boost_LIBRARIES}")
endif()
target_link_libraries(lora ${lora_link_libs})

#
# master
#
include (nodes/master/prg.cmake)
add_executable(master ${node_master})
# libraries to link
set(master_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND master_link_libs "${Boost_LIBRARIES}")
endif()
target_link_libraries(master ${master_link_libs})

#
# modem
#
include (nodes/modem/prg.cmake)
add_executable(modem ${node_modem})
# libraries to link
set(modem_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND modem_link_libs "${Boost_LIBRARIES}")
endif()
target_link_libraries(modem ${modem_link_libs})

#
# setup
#
include (nodes/setup/prg.cmake)
add_executable(setup ${node_setup})
# libraries to link
set(setup_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND setup_link_libs "${Boost_LIBRARIES}")
endif()
target_link_libraries(setup ${setup_link_libs})

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

# setup
configure_file (
	  "${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup.linux.cgf.in"
	  "${PROJECT_BINARY_DIR}/setup_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
configure_file (
	  "${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup.service.in"
	  "${PROJECT_BINARY_DIR}/node-setup.service")
	  
else()
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
