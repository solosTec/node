#
#
cmake_minimum_required (VERSION 3.20)
cmake_policy(SET CMP0074 NEW)
                                                     
#	                                     
#	                               ***   
#	                             ** ***  
#	                            **   *** 
#	                            **       
#	   ****                     **       
#	  * **** * *** **** ****    ******   
#	 **  ****   *** **** ***  * *****    
#	****         **  **** ****  **       
#	  ***        **   **   **   **       
#	    ***      **   **   **   **       
#	      ***    **   **   **   **       
#	 ****  **    **   **   **   **       
#	* **** *     **   **   **   **       
#	   ****      ***  ***  ***  **       
#	              ***  ***  ***  **      
#	                                     

#
# get time stamp of this build
# produce variables _THIS_YEAR and _TWEAK_ID
#
include (cmake/time_stamp.cmake)
                                     
#
# set project name/properties
#
project(smf
	VERSION 0.10.0.${_TWEAK_ID}
	LANGUAGES CXX C
)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    #
    # guard against in-source builds and bad build-type strings
    #
    include(cmake/safeguards.cmake)

    #
    #   print CMake version
    #
    message(STATUS "** CMake              : ${CMAKE_VERSION}")

    math(EXPR "${PROJECT_NAME}_VERSION_NUMERAL" "${PROJECT_VERSION_MAJOR}*100000+${PROJECT_VERSION_MINOR}*1000+${PROJECT_VERSION_PATCH}" OUTPUT_FORMAT DECIMAL)
    message(STATUS "** Version numeral    : ${${PROJECT_NAME}_VERSION_NUMERAL}")

    message(STATUS "** Compiler           : ${CMAKE_CXX_COMPILER_ID} v${CMAKE_CXX_COMPILER_VERSION}")

    # root name of build tree
    set(path_build_tree ${CMAKE_BINARY_DIR})
    cmake_path(GET path_build_tree STEM SMF_BUILD_TREE_STEM)
    message(STATUS "** Build Tree Path    : ${CMAKE_BINARY_DIR}")

    if (NOT WIN32) 
        # /etc/os-release
        # Alpine Linux v3.16/alpine
        cmake_host_system_information(RESULT SMF_HOST_PRETTY_NAME QUERY DISTRIB_PRETTY_NAME)
        cmake_host_system_information(RESULT SMF_HOST_ID_NAME QUERY DISTRIB_ID)
        message(STATUS "** Host System        : ${SMF_HOST_PRETTY_NAME}/${SMF_HOST_ID_NAME} - (${CMAKE_SYSTEM_NAME})")
    else()
        message(STATUS "** Host System        : ${CMAKE_SYSTEM_NAME}")
    endif()

    # Let's ensure -std=c++xx instead of -std=g++xx
    set(CMAKE_CXX_EXTENSIONS OFF)

    # Docs only available if this is the main app
    find_package(Doxygen OPTIONAL_COMPONENTS dot)
    if(Doxygen_FOUND)
        add_subdirectory(docs)
    else()
        message(STATUS "Doxygen not found, not building docs")
    endif()

    if (MSVC)
        message(STATUS "Use static libraries")
        set(GLOBAL_LIBRARY_TYPE STATIC)
        #set(Boost_USE_STATIC_LIBS ON)
        # target is Windows 10
	    add_compile_definitions(_WIN32_WINNT=0x0A00 _SCL_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
        add_compile_options(/bigobj)
        add_compile_options(/MP)
        # __cplusplus is 199711L otherwise
        add_compile_options("/Zc:__cplusplus") 

        # EDITANDCONTINUE
        #add_compile_options("/ZI") 

        message(STATUS "** compile definitions: ${CMAKE_CXX_FLAGS}")

    elseif(CMAKE_COMPILER_IS_GNUCXX)
        set(GLOBAL_LIBRARY_TYPE SHARED)
    endif()

    #
    #   test endian
    #
    include (TestBigEndian)
    TEST_BIG_ENDIAN(${PROJECT_NAME}_BIG_ENDIAN)
    message(STATUS "** big endian         : ${${PROJECT_NAME}_BIG_ENDIAN}")

    #
    #   get an uppercase project name
    #
    string(TOUPPER ${PROJECT_NAME} CAPITAL_NAME)

    #
    # manage unit test: CYNG_UNIT_TEST
    # default is OFF
    #
    if(NOT DEFINED ${CAPITAL_NAME}_UNIT_TEST)
	    set(${CAPITAL_NAME}_UNIT_TEST OFF CACHE BOOL "build unit test")
    endif()

    if(NOT DEFINED ${CAPITAL_NAME}_BUILD_TOOLS)
	    set(${CAPITAL_NAME}_BUILD_TOOLS OFF CACHE BOOL "build developer tools")
    endif()

    if(NOT DEFINED ${CAPITAL_NAME}_TEST_TOOLS)
	    set(${CAPITAL_NAME}_TEST_TOOLS OFF CACHE BOOL "build test tools")
    endif()

endif()


#
# detect address model
#
include (cmake/address_model.cmake)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

#
# find cyng library
# CYNG_INCLUDE_DIRS CYNG_LIBRARIES
#
find_package(CYNG REQUIRED)
if (CYNG_FOUND)
    message(STATUS "** CYNG_INCLUDE_DIRS        : ${CYNG_INCLUDE_DIRS}")
    message(STATUS "** CYNG_LIBRARIES           : ${CYNG_LIBRARIES}")
    message(STATUS "** cyng::db                 : ${CYNG_DB}")
    message(STATUS "** cyng::io                 : ${CYNG_IO}")
    message(STATUS "** cyng::log                : ${CYNG_LOG}")
    message(STATUS "** cyng::net                : ${CYNG_NET}")
    message(STATUS "** cyng::obj                : ${CYNG_OBJ}")
    message(STATUS "** cyng::parse              : ${CYNG_PARSE}")
    message(STATUS "** cyng::rnd                : ${CYNG_RND}")
    message(STATUS "** cyng::sql                : ${CYNG_SQL}")
    message(STATUS "** cyng::store              : ${CYNG_STORE}")
    message(STATUS "** cyng::sys                : ${CYNG_SYS}")
    message(STATUS "** cyng::task               : ${CYNG_TASK}")
    message(STATUS "** cyng::vm                 : ${CYNG_VM}")
    message(STATUS "** cyng::sqlite3            : ${CYNG_SQLITE3}")
	if(WIN32)
        message(STATUS "** cyng::scm                : ${CYNG_SCM}")
    endif(WIN32)
    message(STATUS "** CYNG_PUGI_XML_LIB        : ${CYNG_PUGI_XML_LIB}")
else()
    message(FATAL_ERROR "** CYNG library not found")
endif()

#
# find smfsec library
# -DSMFSEC_ROOT:path=...
#
find_package(SMFSEC REQUIRED)
if (SMFSEC_FOUND)
    message(STATUS "** SMFSEC_INCLUDE_DIRS      : ${SMFSEC_INCLUDE_DIRS}")
    message(STATUS "** SMFSEC_LIBRARIES         : ${SMFSEC_LIBRARIES}")
else()
    message(FATAL_ERROR "** SMFSEC library not found")
endif()

#
#   convert to false/true that can be used in C/C++ code
#	smf_CROSSCOMPILING = true
#
string(TOLOWER ${CMAKE_CROSSCOMPILING} ${PROJECT_NAME}_CROSSCOMPILING)

#   use shared libraries
set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build SHARED libraries")

# Find packages ...

set(SMF_BOOST_COMPONENTS regex iostreams program_options system thread random locale)

#
# unit tests
# call CMake with -DSMF_UNIT_TEST:BOOL=TRUE to generate unit tests 
#
if(${CAPITAL_NAME}_UNIT_TEST)

    list(APPEND SMF_BOOST_COMPONENTS unit_test_framework)

endif()

# Find Boost
set(Boost_NO_BOOST_CMAKE OFF)
find_package(Boost 1.75 
    REQUIRED 
    COMPONENTS ${SMF_BOOST_COMPONENTS}
)
# note: https://github.com/chriskohlhoff/asio/issues/585
add_compile_definitions(BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE BOOST_BIND_NO_PLACEHOLDERS BOOST_SPIRIT_UNICODE BOOST_MPL_LIMIT_LIST_SIZE=50)

# Boost_VERSION => 107700
message(STATUS "** Boost Version                : ${Boost_VERSION}")
# this failes on linux since there is the version string 1.77.0
set(Boost_VERSION_NUMERIC "${Boost_VERSION_MAJOR}.${Boost_VERSION_MINOR}.${Boost_VERSION_PATCH}")
# hard coded for testing - Changed in version 3.15 (CMP0093)
#set(Boost_VERSION_NUMERIC "1.77.0")

message(STATUS "** Boost Version numeric        : ${Boost_VERSION_NUMERIC}")
message(STATUS "** Boost Include                : ${Boost_INCLUDE_DIRS}")
message(STATUS "** Boost Path                   : ${Boost_LIBRARY_DIRS}")
	
message(STATUS "** Boost_LIBRARIES              : ${Boost_LIBRARIES}")
message(STATUS "** Boost_THREAD_LIBRARY         : ${Boost_THREAD_LIBRARY}")
message(STATUS "** Boost_SYSTEM_LIBRARY         : ${Boost_SYSTEM_LIBRARY}")
message(STATUS "** Boost_PROGRAM_OPTIONS_LIBRARY: ${Boost_PROGRAM_OPTIONS_LIBRARY}")
message(STATUS "** Boost_RANDOM_LIBRARY         : ${Boost_RANDOM_LIBRARY}")
message(STATUS "** Boost_REGEX_LIBRARY          : ${Boost_REGEX_LIBRARY}")
if(${CAPITAL_NAME}_UNIT_TEST)
message(STATUS "** Boost_UNIT_TEST_FRAMEWORK    : ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}")
else()
message(STATUS "** Boost_UNIT_TEST_FRAMEWORK    : not installed")
endif()


message(STATUS "** OpenSSL search path          : ${OPENSSL_ROOT_DIR}")
find_package(OpenSSL 1.0.2 REQUIRED)
if(OPENSSL_FOUND)

    add_definitions(-D${PROJECT_NAME}_SSL_INSTALLED)

if (WIN32 AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
    #
    # looks like a regression in CMake 3.24.0
    #
    string(REPLACE "debug;" "Debug;" OPENSSL_LIBRARIES "${OPENSSL_LIBRARIES}")
    string(REPLACE "optimized;" "Release;" OPENSSL_LIBRARIES "${OPENSSL_LIBRARIES}")
    message(STATUS "** openSSL Libraries            : ${OPENSSL_LIBRARIES}")
endif()

#	message(STATUS "** openSSL Found                : ${OPENSSL_FOUND}")
    message(STATUS "** openSSL Version              : ${OPENSSL_VERSION}")
    message(STATUS "** openSSL Include              : ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "** openSSL crypto library       : ${OPENSSL_CRYPTO_LIBRARY}")
    message(STATUS "** openSSL SSL library          : ${OPENSSL_SSL_LIBRARY}")
    message(STATUS "** openSSL Libraries            : ${OPENSSL_LIBRARIES}")
    set(${PROJECT_NAME}_SSL_VERSION ${OPENSSL_VERSION})
#
else()
        set(${PROJECT_NAME}_SSL_VERSION "unknown")
endif()

#
#   get platform
#
include (cmake/platform.cmake)

# FetchContent added in CMake 3.11, downloads during the configure step
include(FetchContent)


#
# add libraries
#

#
# add library: smf_config
#
include (src/lib/config/lib.cmake)
add_library(smf_config ${GLOBAL_LIBRARY_TYPE} ${config_lib})
add_library(smf::config ALIAS "smf_config")

target_include_directories(smf_config
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CYNG_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_config PRIVATE 
    cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug smf config
#
target_compile_definitions(smf_config 
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_INFO>
)

target_link_libraries(smf_config 
    PRIVATE 
    "$<$<PLATFORM_ID:Linux>:cyng::cyng>"
    Boost::boost 
)

#
# add library: smf_cluster
#
include (src/lib/cluster/protocol/lib.cmake)
add_library(smf_cluster ${GLOBAL_LIBRARY_TYPE} ${cluster_lib})
add_library(smf::cluster ALIAS "smf_cluster")

target_include_directories(smf_cluster
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CYNG_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_cluster PRIVATE 
cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug smf cluster
#
target_compile_definitions(smf_cluster 
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_CLUSTER>
)

target_link_libraries(smf_cluster 
    PRIVATE 
        Boost::boost 
)

#
# add library: smf_cluster_bus
#
include (src/lib/cluster/bus/lib.cmake)
add_library(smf_cluster_bus ${GLOBAL_LIBRARY_TYPE} ${cluster_bus})
add_library(smf::cluster_bus ALIAS "smf_cluster_bus")

target_include_directories(smf_cluster_bus
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CYNG_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_cluster_bus PRIVATE 
cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug smf cluster bus
#
target_compile_definitions(smf_cluster_bus 
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_CLUSTER>
)

#
# -Wno-psabi for gcc > 6.0 && gcc < 7.1
#
target_compile_options(smf_cluster_bus 
    PRIVATE 
        $<$<AND:$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},7.0>,$<CXX_COMPILER_ID:GNU>>:-Wno-psabi>
)

target_link_libraries(smf_cluster_bus 
    PRIVATE 
        Boost::boost 
)

#
# add library: smf_http
#
include (src/lib/http/lib.cmake)
add_library(smf_http ${GLOBAL_LIBRARY_TYPE} ${http_lib})
add_library(smf::http ALIAS "smf_http")

target_include_directories(smf_http
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    PRIVATE
        ${CYNG_INCLUDE_DIRS}
        ${SMFSEC_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_http PRIVATE 
    cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug smf http
#
target_compile_definitions(smf_http 
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_HTTP>
)

#
# -Wno-psabi for gcc > 7.1
#
target_compile_options(smf_http 
    PRIVATE 
        $<$<AND:$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},7.1>,$<CXX_COMPILER_ID:GNU>>:-Wno-psabi>
)

target_link_libraries(smf_http 
    PRIVATE 
        Boost::boost 
)

#
# add library: smf_obis
#
include (src/lib/obis/lib.cmake)
add_library(smf_obis ${GLOBAL_LIBRARY_TYPE} ${obis_lib})
add_library(smf::obis ALIAS "smf_obis")

target_include_directories(smf_obis
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CYNG_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_obis PRIVATE 
cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug smf obis
#
target_compile_definitions(smf_obis 
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_OBIS>
)

target_link_libraries(smf_obis 
    PRIVATE 
        Boost::boost 
)

#
# add library: smf_mbus
#
include (src/lib/mbus/protocol/lib.cmake)
add_library(smf_mbus ${GLOBAL_LIBRARY_TYPE} ${mbus_lib})
add_library(smf::mbus ALIAS "smf_mbus")

target_include_directories(smf_mbus
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${OPENSSL_INCLUDE_DIR}
    PRIVATE
        ${CYNG_INCLUDE_DIRS}
        ${SMFSEC_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_mbus PRIVATE 
cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug mbus protocol
#
target_compile_definitions(smf_mbus 
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_MBUS>
)

target_link_libraries(smf_mbus 
    PRIVATE 
        "$<$<PLATFORM_ID:Linux>:${OPENSSL_LIBRARIES}>"
        Boost::boost 
)

#
# add library: smf_iec
#
include (src/lib/iec/protocol/lib.cmake)
add_library(smf_iec ${GLOBAL_LIBRARY_TYPE} ${iec_lib})
add_library(smf::iec ALIAS "smf_iec")


target_include_directories(smf_iec
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${OPENSSL_INCLUDE_DIR}
    PRIVATE
        ${CYNG_INCLUDE_DIRS}
        ${SMFSEC_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_iec PRIVATE 
cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug iec protocol
#
target_compile_definitions(smf_iec 
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_IEC>
)

target_link_libraries(smf_iec 
    PRIVATE 
        Boost::boost 
)

#
# add library: smf_hci
#
include (src/lib/hci/protocol/lib.cmake)
add_library(smf_hci ${GLOBAL_LIBRARY_TYPE} ${hci_lib})
add_library(smf::hci ALIAS "smf_hci")

target_include_directories(smf_hci
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${OPENSSL_INCLUDE_DIR}
    PRIVATE
        ${CYNG_INCLUDE_DIRS}
        ${SMFSEC_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_hci PRIVATE 
cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug hci protocol
#
target_compile_definitions(smf_hci 
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_HCI>
)

target_link_libraries(smf_hci 
    PRIVATE 
        Boost::boost 
)

#
# add library: smf_sml
#
include (src/lib/sml/protocol/lib.cmake)
add_library(smf_sml ${GLOBAL_LIBRARY_TYPE} ${sml_lib})
add_library(smf::sml ALIAS "smf_sml")

target_include_directories(smf_sml
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CYNG_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_sml PRIVATE 
cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug SML protocol
#
target_compile_definitions(smf_sml 
    PRIVATE 
        $<$<CONFIG:Debug>:__DEBUG_SML>
)

target_link_libraries(smf_sml 
    PRIVATE 
#       reference to cyng::crypto::make_rnd_num() required
        "$<$<PLATFORM_ID:Linux>:${cyng_rnd}>" 
#        "$<$<PLATFORM_ID:Linux>:smfsec::smfsec>" 
        Boost::boost 
)

#
# add library: smf_ipt
#
include (src/lib/ipt/protocol/lib.cmake)
add_library(smf_ipt ${GLOBAL_LIBRARY_TYPE} ${ipt_lib})
add_library(smf::ipt ALIAS "smf_ipt")

target_include_directories(smf_ipt
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CYNG_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_ipt PRIVATE 
cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug ipt protocol
#
target_compile_definitions(smf_ipt 
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_IPT>
)

target_link_libraries(smf_ipt 
    PRIVATE 
        Boost::boost 
)

#
# add library: smf_ipt_bus
#
include (src/lib/ipt/bus/lib.cmake)
add_library(smf_ipt_bus ${GLOBAL_LIBRARY_TYPE} ${ipt_bus})
add_library(smf::ipt_bus ALIAS "smf_ipt_bus")

target_include_directories(smf_ipt_bus
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CYNG_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_ipt_bus PRIVATE 
cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug ipt bus
#
target_compile_definitions(smf_ipt_bus 
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_IPT>
)

#
# -Wno-psabi for gcc > 6.0 && gcc < 7.1
#
target_compile_options(smf_ipt_bus 
    PRIVATE 
        $<$<AND:$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},7.0>,$<CXX_COMPILER_ID:GNU>>:-Wno-psabi>
)

target_link_libraries(smf_ipt_bus 
    PRIVATE 
        Boost::boost 
)

#
# add library: smf_modem
#
include (src/lib/modem/protocol/lib.cmake)
add_library(smf_modem ${GLOBAL_LIBRARY_TYPE} ${modem_lib})
add_library(smf::modem ALIAS "smf_modem")

target_include_directories(smf_modem
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CYNG_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_modem PRIVATE 
cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug modem protocol
#
target_compile_definitions(smf_modem 
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_IPT>
)

target_link_libraries(smf_modem 
    PRIVATE 
        Boost::boost 
)

#
# add library: smf_serial_bus
#
include (src/lib/serial/bus/lib.cmake)
add_library(smf_serial_bus ${GLOBAL_LIBRARY_TYPE} ${serial_bus})
add_library(smf::serial_bus ALIAS "smf_serial_bus")

target_include_directories(smf_serial_bus
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CYNG_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_serial_bus PRIVATE 
cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug serial bus
#
target_compile_definitions(smf_serial_bus 
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_SERIAL>
)

target_link_libraries(smf_serial_bus 
    PRIVATE 
        Boost::boost 
)

#
# add library: smf_modbus
#
include (src/lib/modbus/protocol/lib.cmake)
add_library(smf_modbus ${GLOBAL_LIBRARY_TYPE} ${modbus_lib})
add_library(smf::modbus ALIAS "smf_modbus")

target_include_directories(smf_modbus
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${OPENSSL_INCLUDE_DIR}
    PRIVATE
        ${CYNG_INCLUDE_DIRS}
        ${SMFSEC_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_modbus PRIVATE 
    cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug modbus protocol
#
target_compile_definitions(smf_modbus 
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_MODBUS>
)

target_link_libraries(smf_modbus 
    PRIVATE 
        Boost::boost 
)

#
# add library: smf_report
#
include (src/lib/report/lib.cmake)
add_library(smf_report ${GLOBAL_LIBRARY_TYPE} ${report_lib})
add_library(smf::report ALIAS "smf_report")

target_include_directories(smf_report
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${OPENSSL_INCLUDE_DIR}
    PRIVATE
        ${CYNG_INCLUDE_DIRS}
        ${SMFSEC_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(smf_report PRIVATE 
    cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)

#
#   debug report
#
target_compile_definitions(smf_report 
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_REPORT>
)

target_link_libraries(smf_report 
    PRIVATE 
        Boost::boost 
)

#
# nodes
#
add_subdirectory(src/nodes/broker-iec)
add_subdirectory(src/nodes/broker-wmbus)
add_subdirectory(src/nodes/dash)
add_subdirectory(src/nodes/dns)
add_subdirectory(src/nodes/imega)
add_subdirectory(src/nodes/ipt)
add_subdirectory(src/nodes/lora)
add_subdirectory(src/nodes/main)
add_subdirectory(src/nodes/modem)
add_subdirectory(src/nodes/mqtt)
add_subdirectory(src/nodes/report)
add_subdirectory(src/nodes/segw)
add_subdirectory(src/nodes/setup)
add_subdirectory(src/nodes/store)

#
# tools
# call CMake with -DSMF_BUILD_TOOLS:BOOL=TRUE to generate unit tests 
#
if(${CAPITAL_NAME}_BUILD_TOOLS)

    add_subdirectory(src/tools/obis)

endif()

if(${CAPITAL_NAME}_TEST_TOOLS)
    #
    # tests
    # call CMake with -DSMF_TEST_TCP_STRESS:BOOL=TRUE to generate unit tests 
    #
    add_subdirectory(src/tools/tests)
endif()

#
# unit tests
# call CMake with -DSMF_UNIT_TEST:BOOL=TRUE to generate unit tests 
#
if(${CAPITAL_NAME}_UNIT_TEST)

    add_subdirectory(unit_test)

endif()

#
# generate config file
#
configure_file (
    "${PROJECT_SOURCE_DIR}/include/smf.h.in"
    "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}.h"
)
configure_file (
    "${PROJECT_SOURCE_DIR}/include/version.hpp.in"
    "${PROJECT_BINARY_DIR}/include/version.hpp"
)

#
# GeneratING OPKG files requires the OPKG tools (https://git.yoctoproject.org/cgit/cgit.cgi/opkg-utils)
# fakeroot opkg/opkg-tools/opkg-buildpackage
# Install with: opkg --force-space install /tmp/oecp-smf_0.x_armel.ipk
#
message(STATUS "** CMAKE_CROSSCOMPILING         : ${CMAKE_CROSSCOMPILING}")

if(${CMAKE_CROSSCOMPILING})

 # same as ${cmake_project_name_tolower}
	string(TOLOWER ${PROJECT_NAME} OECP_NAME)
    message(STATUS "** cross compile project        : ${OECP_NAME}")

	configure_file (
          "opkg/control.in"
          "${PROJECT_BINARY_DIR}/opkg/control")

    configure_file (
          "opkg/preinst.in"
          "${PROJECT_BINARY_DIR}/opkg/preinst")

	configure_file (
          "opkg/postinst.in"
          "${PROJECT_BINARY_DIR}/opkg/postinst")

    configure_file (
          "opkg/prerm.in"
          "${PROJECT_BINARY_DIR}/opkg/prerm")

    configure_file (
          "opkg/postrm.in"
          "${PROJECT_BINARY_DIR}/opkg/postrm")

	configure_file (
		  "opkg/rules.in"
		  "${PROJECT_BINARY_DIR}/opkg/rules")

endif()

#
# local installation
#
if (WIN32) 
    file(TO_NATIVE_PATH ${smfMain_BINARY_DIR} smfMain_BINARY_DIR)
    file(TO_NATIVE_PATH ${smfMain_BINARY_DIR} smfSetup_BINARY_DIR)
    file(TO_NATIVE_PATH ${smfIPT_BINARY_DIR} smfIPT_BINARY_DIR)
    file(TO_NATIVE_PATH ${smfStore_BINARY_DIR} smfStore_BINARY_DIR)
    file(TO_NATIVE_PATH ${smfDash_BINARY_DIR} smfDash_BINARY_DIR)
    file(TO_NATIVE_PATH ${smfIECBroker_BINARY_DIR} smfIECBroker_BINARY_DIR)
    file(TO_NATIVE_PATH ${smfwMBusBroker_BINARY_DIR} smfwMBusBroker_BINARY_DIR)
    configure_file (
        "${CMAKE_SOURCE_DIR}/scripts/install_local.cmd.in"
        "${PROJECT_BINARY_DIR}/install_local_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cmd")
endif()

#
# install
#

if(UNIX)
include(GNUInstallDirs)
endif()

install(
        DIRECTORY # source directory
            "${CMAKE_SOURCE_DIR}/include/smf/" 
            "${PROJECT_BINARY_DIR}/include/" 
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" # target directory
        FILES_MATCHING # install only matched files
        PATTERN "*.h"
        PATTERN "*hpp"
)

set(SMF_TARGET_LIST smf_cluster smf_cluster_bus smf_config smf_hci smf_http smf_ipt smf_modem smf_iec smf_ipt_bus smf_mbus smf_obis smf_serial_bus smf_sml dash imega ipt segw store lora main modem setup mqtt broker-iec broker-wmbus report)

message(STATUS "** Target list                  : ${SMF_TARGET_LIST}")

install(TARGETS ${SMF_TARGET_LIST}
    EXPORT SMFTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDE_DIR}
)

#
# Packaging
# see https://cliutils.gitlab.io/modern-cmake/chapters/install/installing.html
#
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}/ConfigVersion.cmake"
    VERSION 
        "${CMAKE_PROJECT_VERSION}"
    COMPATIBILITY 
        AnyNewerVersion
)

install(EXPORT SMFTargets
    FILE 
        SMFTargets.cmake
    NAMESPACE 
        smf::
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_PROJECT_NAME}
)

#
# CPack configuration
#
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

set(CPACK_PACKAGE_RELEASE "Hummingbird")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_NAME} - Smart Metering Framework")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION_TWEAK ${PROJECT_VERSION_TWEAK})
set(CPACK_PACKAGE_CONTACT "info@solostec.ch")
set(CPACK_PACKAGE_VENDOR "solosTec")
set(CPACK_PACKAGE_HOMEPAGE_URL "solostec.ch")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PROJECT_NAME}-${CPACK_PROJECT_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")

set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/assets/logo.ico)

if(WIN32)

	set(CPACK_GENERATOR "NSIS64")
	set(CPACK_NSIS_MUI_ICON ${PROJECT_SOURCE_DIR}\\assets\\logo.ico)
	set(CPACK_NSIS_MUI_UNIICON ${PROJECT_SOURCE_DIR}\\assets\\logo.ico)
	#set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\vcredist_x86.exe\\\" /q:a'")
	set(CPACK_NSIS_MODIFY_PATH ON)
	set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.solostec.com")
else()

    set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

    if (${SMF_HOST_ID_NAME} STREQUAL "openSUSE")
        message(STATUS "** Generator                    : RPM")
        set(CPACK_GENERATOR "RPM")
        set(CPACK_SOURCE_GENERATOR "RPM")
        set(CPACK_RPM_PACKAGE_DESCRIPTION "SMF framework")
    else()    

#   https://cmake.org/cmake/help/v3.13/cpack_gen/deb.html

        message(STATUS "** Generator                    : DEB")
	    set(CPACK_GENERATOR "DEB")
        set(CPACK_SOURCE_GENERATOR "DEB")
#        set(CPACK_DEBIAN_PACKAGE_DEPENDS "Boost")
        set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
        set(CPACK_DEBIAN_COMPRESSION_TYPE "gzip")
    endif()    
endif()

include(CPack)
