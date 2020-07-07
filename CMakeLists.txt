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
#	programm info info
#
include (lib/info/lib.cmake)
add_library(smf_info ${GLOBAL_LIBRARY_TYPE} ${info_lib})
target_include_directories(smf_info
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

#
#	cluster library (client side)
#
include (lib/cluster/lib.cmake)
add_library(smf_cluster ${GLOBAL_LIBRARY_TYPE} ${cluster_lib})
target_include_directories(smf_cluster
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

#
#	ipt protocol library
#
include (lib/ipt/protocol/lib.cmake)
add_library(smf_protocol_ipt ${GLOBAL_LIBRARY_TYPE} ${ipt_protocol_lib})
target_include_directories(smf_protocol_ipt
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

#
#	ipt bus library (client)
#
include (lib/ipt/bus/lib.cmake)
add_library(smf_bus_ipt ${GLOBAL_LIBRARY_TYPE} ${ipt_bus_lib})
target_include_directories(smf_bus_ipt
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

#
#	sml protocol library
#
include (lib/sml/protocol/lib.cmake)
add_library(smf_protocol_sml ${GLOBAL_LIBRARY_TYPE} ${sml_protocol_lib})
target_include_directories(smf_protocol_sml
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${OPENSSL_INCLUDE_DIR}
)

#
#	sml bus library (client)
#
include (lib/sml/bus/lib.cmake)
add_library(smf_bus_sml ${GLOBAL_LIBRARY_TYPE} ${sml_bus_lib})
target_include_directories(smf_bus_sml
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

#
#	modem/AT protocol library
#
include (lib/modem/protocol/lib.cmake)
add_library(smf_protocol_modem ${GLOBAL_LIBRARY_TYPE} ${modem_protocol_lib})
target_include_directories(smf_protocol_modem
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

#
#	M-Bus/EN 13757-3 protocol/application layer library
#
include (lib/mbus/protocol/lib.cmake)
add_library(smf_protocol_mbus ${GLOBAL_LIBRARY_TYPE} ${mbus_protocol_lib})
target_include_directories(smf_protocol_mbus
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${CRYPT_INCLUDE_DIR}
		${OPENSSL_INCLUDE_DIR}
)

#
#	iMega/CU protocol library
#
include (lib/imega/protocol/lib.cmake)
add_library(smf_protocol_imega ${GLOBAL_LIBRARY_TYPE} ${imega_protocol_lib})
target_include_directories(smf_protocol_imega
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

#
#	HCI/CP210x protocol library
#
include (lib/hci/protocol/lib.cmake)
add_library(smf_protocol_hci ${GLOBAL_LIBRARY_TYPE} ${hci_protocol_lib})
target_include_directories(smf_protocol_hci
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

#
#	IEC 62056-21 protocol library
#
include (lib/iec/protocol/lib.cmake)
add_library(smf_protocol_iec ${GLOBAL_LIBRARY_TYPE} ${iec_protocol_lib})
target_include_directories(smf_protocol_iec
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

#
#	serial bus library (client)
#	at it's core it's the boost asio serial_port implementation
#
include (lib/serial/bus/lib.cmake)
add_library(smf_bus_serial ${GLOBAL_LIBRARY_TYPE} ${serial_bus_lib})
target_include_directories(smf_bus_serial
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

#
#	telnet protocol library
#
include (lib/telnet/protocol/lib.cmake)
add_library(smf_protocol_telnet ${GLOBAL_LIBRARY_TYPE} ${telnet_protocol_lib})
target_include_directories(smf_protocol_telnet
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

#
#	LoRa payload library
#
include (lib/lora/payload/lib.cmake)
add_library(smf_lora ${GLOBAL_LIBRARY_TYPE} ${lora_payload_lib})
target_include_directories(smf_lora
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)

#
#	http server library
#
include (lib/http/server/lib.cmake)
add_library(smf_http_srv ${GLOBAL_LIBRARY_TYPE} ${http_srv_lib})
target_include_directories(smf_http_srv
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${BOOST_INCLUDE_DIR}
		${CYNG_INCLUDE_DIRS}
		${CRYPT_INCLUDE_DIR}
)

#
# https server library
#
if(${PROJECT_NAME}_SSL_SUPPORT)
	include (lib/https/server/lib.cmake)
	add_library(smf_https_srv ${GLOBAL_LIBRARY_TYPE} ${https_srv_lib})
	target_include_directories(smf_https_srv
		PRIVATE
			"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
			${CYNG_INCLUDE_DIRS}
			${OPENSSL_INCLUDE_DIR}
			${CRYPT_INCLUDE_DIR}
	)
	
endif()

#
#	install libraries
#
#
# List of all SMF libraries
#
set(SMF_LIB_LIST smf_info smf_http_srv smf_protocol_ipt smf_bus_ipt smf_protocol_sml smf_bus_sml smf_protocol_modem smf_protocol_mbus smf_protocol_imega smf_protocol_iec smf_bus_serial smf_protocol_telnet smf_http_srv)
if(${PROJECT_NAME}_SSL_SUPPORT)
    list(APPEND SMF_LIB_LIST smf_https_srv)
endif()
install(TARGETS ${SMF_LIB_LIST}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static)


if(${PROJECT_NAME}_BUILD_TEST AND ${PROJECT_NAME}_SSL_SUPPORT)

	include (nodes/www/prg.cmake)
#
# This program is only for testing puposes
# generic HTTP server
#

	add_executable(http ${node_http})
	target_include_directories(http
		PRIVATE
			"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
			${BOOST_INCLUDE_DIR}
			${CYNG_INCLUDE_DIRS}
			${CRYPT_INCLUDE_DIR}
	)
	target_link_libraries(http 
		smf_info 
		smf_http_srv
		${CYNG_LIBRARIES} 
		${OPENSSL_LIBRARIES}
		${SMF_CRYPTO_LIBRARY}
		"$<$<PLATFORM_ID:Linux>:${Boost_LIBRARIES};pthread>"
	)
	
#
# This program is only for testing puposes
# generic HTTPS server
#
	add_executable(https ${node_https})
	target_include_directories(https
		PRIVATE
			"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
			${CYNG_INCLUDE_DIRS}
			${OPENSSL_INCLUDE_DIR}
			${CRYPT_INCLUDE_DIR}
			${BOOST_INCLUDE_DIR}
	)
	target_link_libraries(https 
		smf_info 
		smf_https_srv
		${CYNG_LIBRARIES} 
		${OPENSSL_LIBRARIES}
		${SMF_CRYPTO_LIBRARY}
		"$<$<PLATFORM_ID:Linux>:${Boost_LIBRARIES};pthread>"
	)
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
	target_include_directories(assl
		PRIVATE
			"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
			${OPENSSL_INCLUDE_DIR}
#			${BOOST_INCLUDE_DIR}
	)
	target_link_libraries(assl 
		${OPENSSL_LIBRARIES}
#		${SMF_CRYPTO_LIBRARY}
		"$<$<PLATFORM_ID:Linux>:${Boost_LIBRARIES};pthread>"
	)	
endif()

if(${PROJECT_NAME}_SSL_SUPPORT)
#
# dashboard (HTTP)
#
    include (nodes/admin/prg.cmake)
    add_executable(dash ${node_dash})
	target_include_directories(dash
		PRIVATE
			"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
			${CYNG_INCLUDE_DIRS}
			${CRYPT_INCLUDE_DIR}
			${BOOST_INCLUDE_DIR}
			${PUGIXML_INCLUDE_DIR}
	)
	target_link_libraries(dash 
		smf_info 
		smf_cluster
		smf_http_srv
		${SMF_CRYPTO_LIBRARY}
		${CYNG_LIBRARIES} 
		${OPENSSL_LIBRARIES}
		"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};pthread>"
        $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
		"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
	)

#
# dashboard (HTTPS)
#
    add_executable(dashs ${node_dashs})
	target_include_directories(dashs
		PRIVATE
			"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
			${CYNG_INCLUDE_DIRS}
			${OPENSSL_INCLUDE_DIR}
			${CRYPT_INCLUDE_DIR}
			${BOOST_INCLUDE_DIR}
			${PUGIXML_INCLUDE_DIR}
	)
	target_link_libraries(dashs 
		smf_info 
		smf_cluster
		smf_https_srv
		${SMF_CRYPTO_LIBRARY}
		${CYNG_LIBRARIES} 
		${OPENSSL_LIBRARIES}
		"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};pthread>"
        $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
		"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
	)
endif()

#
# e350
#
include (nodes/e350/prg.cmake)
add_executable(e350 ${node_e350})
target_include_directories(e350
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(e350 
	smf_info 
	smf_cluster
	smf_protocol_imega
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};pthread>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)

#
# IP-T collector
#
include (nodes/ipt/collector/prg.cmake)
add_executable(collector ${node_ipt_collector})
target_include_directories(collector
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(collector 
	smf_info 
	smf_protocol_ipt
	smf_bus_ipt
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};pthread>"
	$<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)

#
# IP-T emitter
#
include (nodes/ipt/emitter/prg.cmake)
add_executable(emitter ${node_ipt_emitter})
target_include_directories(emitter
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(emitter 
	smf_info 
	smf_protocol_ipt
	smf_bus_ipt
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};pthread>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)


#
# IP-T SEGW (gateway)
#
include (nodes/ipt/segw/prg.cmake)
add_executable(segw ${node_ipt_segw})
target_include_directories(segw
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${CRYPT_INCLUDE_DIR}
		${OPENSSL_INCLUDE_DIR}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(segw 
	smf_info
	smf_protocol_ipt 
	smf_bus_ipt 
	smf_bus_sml 
	smf_bus_serial 
	smf_protocol_sml 
	smf_protocol_mbus 
	smf_protocol_hci 
	${CYNG_LIBRARIES} 
	${SMF_CRYPTO_LIBRARY}
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${SQLite3_LIBRARY};${CMAKE_DL_LIBS};pthread>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)

#
# IP-T master
#
include (nodes/ipt/master/prg.cmake)
add_executable(ipt ${node_ipt_master})
target_include_directories(ipt
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(ipt 
	smf_info 
	smf_cluster 
	smf_protocol_ipt 
	smf_protocol_sml
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};pthread>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

#
# IP-Ts master
#
add_executable(ipts ${node_ipts_master})
target_include_directories(ipts
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(ipts 
	smf_info 
	smf_cluster 
	smf_protocol_ipt 
	smf_protocol_sml
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};pthread>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

#
# IP-T store
#
include (nodes/ipt/store/prg.cmake)
add_executable(store ${node_ipt_store})
target_include_directories(store
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(store 
	smf_info 
	smf_protocol_ipt 
	smf_bus_ipt 
	smf_protocol_sml 
	smf_protocol_iec 
	smf_protocol_mbus 
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};pthread>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)
	
if(${PROJECT_NAME}_BUILD_TEST)
#
# This program is only for testing purposes
# IP-T stress
#
	include (nodes/ipt/stress/prg.cmake)
	add_executable(stress ${node_ipt_stress})
	target_include_directories(stress
		PRIVATE
			"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
			${CYNG_INCLUDE_DIRS}
			${BOOST_INCLUDE_DIR}
	)
	target_link_libraries(stress 
		smf_protocol_ipt 
		smf_bus_ipt 
		smf_protocol_sml
		${CYNG_LIBRARIES} 
		"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};pthread>"
        $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
		"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
	)
endif()

#
# LoRa
#
if(${PROJECT_NAME}_SSL_SUPPORT)

    include (nodes/lora/prg.cmake)
    add_executable(lora ${node_lora})
	target_include_directories(lora
		PRIVATE
			"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
			${CYNG_INCLUDE_DIRS}
			${OPENSSL_INCLUDE_DIR}
			${BOOST_INCLUDE_DIR}
	)
	target_link_libraries(lora 
		smf_info 
		smf_cluster 
		smf_https_srv 
		smf_lora 
		smf_protocol_sml
		${OPENSSL_LIBRARIES}
		${SMF_CRYPTO_LIBRARY}
		${CYNG_LIBRARIES} 
		"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};pthread>"
        $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	)
endif()

#
# master node
#
include (nodes/master/prg.cmake)
add_executable(master ${node_master})
target_include_directories(master
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(master 
	smf_info 
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};atomic;pthread>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

#
# modem node
#
include (nodes/modem/prg.cmake)
add_executable(modem ${node_modem})
target_include_directories(modem
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(modem 
	smf_info 
	smf_cluster 
	smf_protocol_sml 
	smf_protocol_modem 
	${CYNG_LIBRARIES}
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};pthread>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

#
# mqtt
# requires MSVC 19 of GCC > 7.x
#
if(MSVC)
    include (nodes/mqtt/prg.cmake)
    add_executable(mqtt ${node_mqtt})

    target_compile_options(mqtt 
		PRIVATE 
			"-DMQTT_NO_TLS")

    target_include_directories(mqtt 
		PRIVATE 
			"${PROJECT_SOURCE_DIR}/3party"
			${CYNG_INCLUDE_DIRS}
			"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
	)
    # libraries to link
    target_link_libraries(mqtt 
		smf_info 
		smf_cluster 
		${CYNG_LIBRARIES}
		"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};pthread>"
	)
endif(MSVC)

#
# iec_62056
#
include (nodes/iec-62056/prg.cmake)
add_executable(iec_62056 ${node_iec_62056})
target_include_directories(iec_62056
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(iec_62056 
	smf_info 
	smf_cluster 
	${CYNG_LIBRARIES}
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};pthread>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

#
# setup
#
include (nodes/setup/prg.cmake)
add_executable(setup ${node_setup})
target_include_directories(setup
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${CRYPT_INCLUDE_DIR}
		${OPENSSL_INCLUDE_DIR}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(setup 
	smf_info 
	smf_cluster 
	${SMF_CRYPTO_LIBRARY}
	${CYNG_LIBRARIES}
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};${SQLite3_LIBRARY};pthread>"
	$<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)

#
# task: csv
#
include (tasks/csv/prg.cmake)
add_executable(csv ${task_csv})
target_include_directories(csv
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(csv 
#	smf_info 
	smf_protocol_sml
	smf_cluster 
	${CYNG_LIBRARIES}
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};${SQLite3_LIBRARY};pthread>"
	$<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)

#
# task: tsdb (time series database)
#
include (tasks/tsdb/prg.cmake)
add_executable(tsdb ${task_tsdb})
target_include_directories(tsdb
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(tsdb 
#	smf_info 
	smf_cluster 
	${CYNG_LIBRARIES}
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};pthread>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

#
# task: stat (statistics - detecting gaps, etc)
#
include (tasks/stat/prg.cmake)
add_executable(stat ${task_stat})
target_include_directories(stat
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(stat 
	smf_info 
	smf_cluster 
	${OPENSSL_LIBRARIES}
	${CYNG_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_LIBRARIES};pthread;${CMAKE_DL_LIBS}>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

#
# test unit using Boost.Test
# BOOST_TEST_DYN_LINK is required to build a main() function
# cmake -DNODE_BUILD_TEST:bool=ON ..
#

if(${PROJECT_NAME}_BUILD_TEST AND ${PROJECT_NAME}_SSL_SUPPORT)
	include (test/unit-test/unit-test.cmake)
	add_executable(unit_test ${unit_test})
	target_include_directories(unit_test
		PRIVATE
			"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
			${CYNG_INCLUDE_DIRS}
			${BOOST_INCLUDE_DIR}
			${OPENSSL_INCLUDE_DIR}
			${CRYPT_INCLUDE_DIR}
			${PUGIXML_INCLUDE_DIR}
	)
		
# 	set_property(
# 		TARGET unit_test
# 		PROPERTY COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK BOOST_ASIO_HAS_MOVE)

	# BOOST_TEST_DYN_LINK is required to build a main() function
	target_compile_definitions(unit_test
		PRIVATE
			BOOST_TEST_DYN_LINK BOOST_ASIO_HAS_MOVE)

	target_link_libraries(unit_test 
		smf_protocol_ipt 
		smf_bus_ipt 
		smf_protocol_sml 
		smf_protocol_mbus 
		smf_bus_serial
		${CYNG_LIBRARIES}
		${OPENSSL_LIBRARIES}
		"$<$<PLATFORM_ID:Linux>:${SQLite3_LIBRARY};${Boost_UNIT_TEST_FRAMEWORK_LIBRARY};${Boost_LIBRARIES};pthread;${CMAKE_DL_LIBS}>"
		"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
        $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	)
endif()

#
# SMF tool
#
if(NOT ${PROJECT_NAME}_CROSS_COMPILE)
	include (tools/smf/prg.cmake)
	add_executable(smf ${tool_smf})
	target_include_directories(smf
		PRIVATE
			"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/main/include;${CMAKE_CURRENT_BINARY_DIR}>"
			${CYNG_INCLUDE_DIRS}
			${BOOST_INCLUDE_DIR}
			${CRYPT_INCLUDE_DIR}
			${PUGIXML_INCLUDE_DIR}
	)
	# libraries to link 
	target_link_libraries(smf 
		smf_info 
		smf_protocol_sml 
		smf_protocol_mbus 
		${CYNG_LIBRARIES} 
		${OPENSSL_LIBRARIES}
		"$<$<PLATFORM_ID:Linux>:${Boost_LIBRARIES};pthread;readline>"
        $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	)
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
