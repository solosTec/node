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
	set(CMAKE_CXX_FLAGS "-D_WIN32_WINNT=0x0601 -D_SCL_SECURE_NO_WARNINGS -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE")	
	add_definitions(/EHsc /bigobj /MP /std:c++latest)
	
else()

	message( FATAL_ERROR "Unknown or missing compiler: ${CMAKE_CXX_COMPILER_ID}" )
	
endif()

#
#	setup Boost library
#

if(UNIX)
    if(EXISTS "$ENV{HOME}/projects/boost_1_67_0/install")
        set(BOOST_ROOT "$ENV{HOME}/projects/boost_1_67_0/install" CACHE PATH "BOOST_ROOT")
        set(BOOST_LIBRARYDIR "$ENV{HOME}/projects/boost_1_67_0/install/lib" CACHE PATH "BOOST_LIBRARYDIR")
        message(STATUS "** Search Boost    : overwrite with ${BOOST_ROOT}")
    elseif(EXISTS "$ENV{HOME}/projects/boost_1_66_0/install")
        set(BOOST_ROOT "$ENV{HOME}/projects/boost_1_66_0/install" CACHE PATH "BOOST_ROOT")
        set(BOOST_LIBRARYDIR "$ENV{HOME}/projects/boost_1_66_0/install/lib" CACHE PATH "BOOST_LIBRARYDIR")
        message(STATUS "** Search Boost    : overwrite with ${BOOST_ROOT}")
    endif()
endif(UNIX)

set(Boost_ADDITIONAL_VERSIONS "1.65.0" "1.66.0" "1.67.0")
message(STATUS "** Search Boost    : ${Boost_ADDITIONAL_VERSIONS}")
find_package(Boost 1.66 REQUIRED COMPONENTS thread system date_time filesystem program_options random signals unit_test_framework regex)

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
	
	set(${PROJECT_NAME}_BOOST_LIBRARY ${PROJECT_NAME}_BOOST_LIBRARY_FOUND)
	set(${PROJECT_NAME}_BOOST_VERSION ${PROJECT_NAME}_BOOST_${Boost_VERSION})
	
	# increase MPL list size for Boost.Variant
	# possible values are  30, 40 or 50 
	#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
	#define BOOST_MPL_LIMIT_LIST_SIZE 30
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_MPL_CFG_NO_PREPROCESSED_HEADERS -DBOOST_MPL_LIMIT_LIST_SIZE=50 -DBOOST_ASIO_ENABLE_HANDLER_TRACKING")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_MPL_CFG_NO_PREPROCESSED_HEADERS -DBOOST_MPL_LIMIT_LIST_SIZE=50")

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
		link_directories(${cyng_LIBRARY_DIRS})
 	endif(CROSS_COMPILE)
	message(STATUS "** cyng Libraries   : ${cyng_LIBRARY_DIRS}")
else()
	#
	#	$(ConfigurationName) is a variable used by the MS build system
	#
 	get_filename_component(cyng_LIBRARY_DIRS "${PROJECT_BINARY_DIR}/../cyng" REALPATH)
 	get_filename_component(cyng_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/../cyng/src/main/include" REALPATH)
	set(cyng_INCLUDE_DIRS "${cyng_LIBRARY_DIRS}" ${cyng_INCLUDE_DIR})
	include_directories(${cyng_INCLUDE_DIRS})
	link_directories(${cyng_LIBRARY_DIRS}/$(ConfigurationName))

	message(STATUS "** CYNG Include Paths    : ${cyng_INCLUDE_DIRS}")
	message(STATUS "** CYNG Library Paths    : ${cyng_LIBRARY_DIRS}/$(ConfigurationName)")
endif()

#
#	Use CMake modules from CYNG project
#
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/../cyng/src/modules/")
message(STATUS "** CMake modules         :  ${CMAKE_MODULE_PATH}")

#
# libpugixml-dev (v1.8)
# On Windows set CMake variables PugiXML_INCLUDE_DIRS and PugiXML_LIBRARIES 
#
find_package(PugiXML)
if(PugiXML_FOUND)

	message(STATUS "** PugiXML Include   : ${PUGIXML_INCLUDE_DIR}")
	message(STATUS "** PugiXML Libraries : ${PUGIXML_LIBRARY}")
	set(${PROJECT_NAME}_PUGIXML_INSTALLED ON)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${PROJECT_NAME}_PUGIXML_INSTALLED" )
	include_directories(${PUGIXML_INCLUDE_DIR})

else()

	message(WARNING "** PugiXML not found - don't build XML lib")
	set(${PROJECT_NAME}_PUGIXML_INSTALLED OFF)

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
	target_link_libraries(smf_protocol_sml cyng_vm)
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
#	http server library
#
include (lib/http/server/lib.cmake)
add_library(smf_http_srv ${GLOBAL_LIBRARY_TYPE} ${http_srv_lib})

#
# https server library
#
include (lib/https/server/lib.cmake)
add_library(smf_https_srv ${GLOBAL_LIBRARY_TYPE} ${https_srv_lib})

#
# generic HTTP server
#
include (nodes/http/prg.cmake)
add_executable(http ${node_http})
# libraries to link 
set(http_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_mail cyng_crypto cyng_sys ${OPENSSL_LIBRARIES})
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND http_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND http_link_libs pthread)
    endif()
endif()
target_link_libraries(http ${http_link_libs})
	
#
# generic HTTPS server
#
include (nodes/https/prg.cmake)
add_executable(https ${node_https})
# libraries to link 
set(https_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_crypto cyng_sys smf_https_srv ${OPENSSL_LIBRARIES})
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND https_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND https_link_libs pthread)
    endif()
endif()
target_link_libraries(https ${https_link_libs})

#
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

#
# dashboard
#
include (nodes/dash/prg.cmake)
add_executable(dash ${node_dash})
# libraries to link
set(dashboard_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_store cyng_table cyng_sys smf_cluster smf_http_srv)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND dashboard_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND dashboard_link_libs pthread)
    endif()
endif()
target_link_libraries(dash ${dashboard_link_libs})

#
# e350
#
include (nodes/e350/prg.cmake)
add_executable(e350 ${node_e350})
# libraries to link
set(e350_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain smf_cluster)
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
set(collector_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_table smf_protocol_ipt smf_bus_ipt)
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
set(emitter_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_table smf_protocol_ipt smf_bus_ipt)
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
set(gateway_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain smf_protocol_ipt smf_bus_ipt smf_bus_sml)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND gateway_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND gateway_link_libs pthread)
    endif()
endif()
if(WIN32)
	list(APPEND gateway_link_libs odbc32.lib lib_sqlite3)
else()
	list(APPEND gateway_link_libs ${SQLite3_LIBRARY})
endif(WIN32)
if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	list(APPEND gateway_link_libs ${PUGIXML_LIBRARY})
endif()

target_link_libraries(gateway ${gateway_link_libs})

#
# IP-T master
#
include (nodes/ipt/master/prg.cmake)
add_executable(ipt ${node_ipt_master})
# libraries to link
set(ipt_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain smf_cluster smf_protocol_ipt)
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
set(store_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_db cyng_sql cyng_store cyng_table cyng_xml smf_protocol_ipt smf_bus_ipt smf_protocol_sml)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND store_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND store_link_libs pthread)
    endif()
endif()
if(WIN32)
	list(APPEND store_link_libs odbc32.lib lib_sqlite3)
else()
	list(APPEND store_link_libs ${SQLite3_LIBRARY})
endif(WIN32)
if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	list(APPEND store_link_libs ${PUGIXML_LIBRARY})
endif()
#message(STATUS "** link ipt_store        : ${store_link_libs}")
target_link_libraries(store ${store_link_libs})
	
#
# lora
#
include (nodes/lora/prg.cmake)
add_executable(lora ${node_lora})
# libraries to link
set(lora_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain smf_cluster)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND lora_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND lora_link_libs pthread)
    endif()
endif()
#message(STATUS "** link lora             : ${lora_link_libs}")
target_link_libraries(lora ${lora_link_libs})

#
# master
#
include (nodes/master/prg.cmake)
add_executable(master ${node_master})
# libraries to link
set(master_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_store cyng_table cyng_vm cyng_domain)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND master_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND master_link_libs pthread)
    endif()
endif()
#message(STATUS "** link master           : ${master_link_libs}")
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
# libraries to link
set(mqtt_link_libs cyng_core cyng_io cyng_async cyng_log cyng_json cyng_parser cyng_vm cyng_domain cyng_table smf_cluster)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND mqtt_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND mqtt_link_libs pthread)
    endif()
endif()
#message(STATUS "** link mqtt             : ${mqtt_link_libs}")
target_link_libraries(mqtt ${mqtt_link_libs})

#
# setup
#
include (nodes/setup/prg.cmake)
add_executable(setup ${node_setup})
# libraries to link
set(setup_link_libs cyng_core cyng_io cyng_async cyng_log cyng_parser cyng_json cyng_vm cyng_domain cyng_db cyng_sql cyng_store cyng_table smf_cluster)
if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
	list(APPEND setup_link_libs "${Boost_LIBRARIES}")
    if (UNIX)
        list(APPEND setup_link_libs pthread)
    endif()
endif()
if(WIN32)
	list(APPEND setup_link_libs odbc32.lib lib_sqlite3)
else()
	list(APPEND setup_link_libs ${SQLite3_LIBRARY})
endif(WIN32)
#message(STATUS "** link setup            : ${setup_link_libs}")
target_link_libraries(setup ${setup_link_libs})

#
# test unit using Boost.Test
# BOOST_TEST_DYN_LINK is required to build a main() function
#
if(${PROJECT_NAME}_BUILD_TEST)
include (test/unit-test/unit-test.cmake)
add_executable(unit_test ${unit_test})
	
# BOOST_TEST_DYN_LINK is required to build a main() function
set_property(
	TARGET unit_test
	PROPERTY COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK BOOST_ASIO_HAS_MOVE)

set(unittest_link_libs cyng_core cyng_io cyng_async cyng_log cyng_store cyng_vm cyng_domain cyng_sql cyng_json cyng_parser cyng_mail cyng_crypto cyng_sys cyng_db cyng_table cyng_xml smf_protocol_ipt smf_protocol_sml)

if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	list(APPEND unittest_link_libs ${PUGIXML_LIBRARY} cyng_xml)
endif()

if (${GLOBAL_LIBRARY_TYPE} STREQUAL "SHARED")
#	list(APPEND unittest_link_libs "${Boost_LIBRARIES} ${OPENSSL_LIBRARIES}")
	list(APPEND unittest_link_libs ${Boost_LIBRARIES})
    if (UNIX)
        list(APPEND unittest_link_libs pthread)
    endif()
endif()

#message(STATUS "** link unit-test        : ${unittest_link_libs}")
arget_link_libraries(unit_test ${unittest_link_libs})
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
	  
else()


	# http
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/http/templates/http.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/http_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/http/templates/http_create_service.bat.in"
		  "${PROJECT_BINARY_DIR}/http_create_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/http/templates/http_delete_service.bat.in"
		  "${PROJECT_BINARY_DIR}/http_delete_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/http/templates/http_restart_service.bat.in"
		  "${PROJECT_BINARY_DIR}/http_restart_service.bat")

	# dash
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dash/templates/dash.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/dash_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dash/templates/dash_create_service.bat.in"
		  "${PROJECT_BINARY_DIR}/dash_create_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dash/templates/dash_delete_service.bat.in"
		  "${PROJECT_BINARY_DIR}/dash_delete_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/dash/templates/dash_restart_service.bat.in"
		  "${PROJECT_BINARY_DIR}/dash_restart_service.bat")

	# e355
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/e350_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350_create_service.bat.in"
		  "${PROJECT_BINARY_DIR}/e350_create_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350_delete_service.bat.in"
		  "${PROJECT_BINARY_DIR}/e350_delete_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/e350/templates/e350_restart_service.bat.in"
		  "${PROJECT_BINARY_DIR}/e350_restart_service.bat")

	# ipt collector 
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/collector_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector_create_service.bat.in"
		  "${PROJECT_BINARY_DIR}/collector_create_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector_delete_service.bat.in"
		  "${PROJECT_BINARY_DIR}/collector_delete_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/collector/templates/collector_restart_service.bat.in"
		  "${PROJECT_BINARY_DIR}/collector_restart_service.bat")

	# ipt emitter 
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/emitter_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter_create_service.bat.in"
		  "${PROJECT_BINARY_DIR}/emitter_create_service.bat")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter_delete_service.bat.in"
		  "${PROJECT_BINARY_DIR}/emitter_delete_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/emitter/templates/emitter_restart_service.bat.in"
		  "${PROJECT_BINARY_DIR}/emitter_restart_service.bat")

	# ipt gateway
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/gateway_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway_create_service.bat.in"
		  "${PROJECT_BINARY_DIR}/gateway_create_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway_delete_service.bat.in"
		  "${PROJECT_BINARY_DIR}/gateway_delete_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/gateway/templates/gateway_restart_service.bat.in"
		  "${PROJECT_BINARY_DIR}/gateway_restart_service.bat")

	# ipt master
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/ipt_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt_create_service.bat.in"
		  "${PROJECT_BINARY_DIR}/ipt_create_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt_delete_service.bat.in"
		  "${PROJECT_BINARY_DIR}/gateway_delete_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/master/templates/ipt_restart_service.bat.in"
		  "${PROJECT_BINARY_DIR}/gateway_restart_service.bat")

	# ipt store
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/store_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store_create_service.bat.in"
		  "${PROJECT_BINARY_DIR}/store_create_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store_delete_service.bat.in"
		  "${PROJECT_BINARY_DIR}/store_delete_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/ipt/store/templates/store_restart_service.bat.in"
		  "${PROJECT_BINARY_DIR}/store_restart_service.bat")

	# lora
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/lora_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora_create_service.bat.in"
		  "${PROJECT_BINARY_DIR}/lora_create_service.bat")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora_delete_service.bat.in"
		  "${PROJECT_BINARY_DIR}/lora_delete_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/lora/templates/lora_restart_service.bat.in"
		  "${PROJECT_BINARY_DIR}/lora_restart_service.bat")

	# master
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/master/templates/master.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/master_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/master/templates/master_create_service.bat.in"
		  "${PROJECT_BINARY_DIR}/master_create_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/master/templates/master_delete_service.bat.in"
		  "${PROJECT_BINARY_DIR}/master_delete_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/master/templates/master_restart_service.bat.in"
		  "${PROJECT_BINARY_DIR}/master_restart_service.bat")

	# modem
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/modem_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem_create_service.bat.in"
		  "${PROJECT_BINARY_DIR}/modem_create_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem_delete_service.bat.in"
		  "${PROJECT_BINARY_DIR}/modem_delete_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/modem/templates/modem_restart_service.bat.in"
		  "${PROJECT_BINARY_DIR}/modem_restart_service.bat")

	# mqtt
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/mqtt_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt_create_service.bat.in"
		  "${PROJECT_BINARY_DIR}/mqtt_create_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt_delete_service.bat.in"
		  "${PROJECT_BINARY_DIR}/mqtt_delete_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/mqtt/templates/mqtt_restart_service.bat.in"
		  "${PROJECT_BINARY_DIR}/mqtt_restart_service.bat")

	# setup
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup.windows.cgf.in"
		  "${PROJECT_BINARY_DIR}/setup_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")
	  
	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup_create_service.bat.in"
		  "${PROJECT_BINARY_DIR}/setup_create_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup_delete_service.bat.in"
		  "${PROJECT_BINARY_DIR}/setup_delete_service.bat")

	configure_file (
		  "${PROJECT_SOURCE_DIR}/nodes/setup/templates/setup_restart_service.bat.in"
		  "${PROJECT_BINARY_DIR}/setup_restart_service.bat")

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
