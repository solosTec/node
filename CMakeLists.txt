#
#
cmake_minimum_required (VERSION 3.13)

                                                     
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
	VERSION 0.9.0.1
	LANGUAGES CXX C
)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    #
    # guard against in-source builds and bad build-type strings
    #
    include(cmake/safeguards.cmake)

    math(EXPR "${PROJECT_NAME}_VERSION_NUMERAL" "${PROJECT_VERSION_MAJOR}*100000+${PROJECT_VERSION_MINOR}*1000+${PROJECT_VERSION_PATCH}" OUTPUT_FORMAT DECIMAL)
    message(STATUS "** Version numeral    : ${${PROJECT_NAME}_VERSION_NUMERAL}")

    message(STATUS "** Compiler           : ${CMAKE_CXX_COMPILER_ID} v${CMAKE_CXX_COMPILER_VERSION}")


    # Let's ensure -std=c++xx instead of -std=g++xx
    set(CMAKE_CXX_EXTENSIONS OFF)

    # Docs only available if this is the main app
    find_package(Doxygen)
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
    message(STATUS "** cyng_db                  : ${cyng_db}")
    message(STATUS "** cyng_io                  : ${cyng_io}")
    message(STATUS "** cyng_log                 : ${cyng_log}")
    message(STATUS "** cyng_obj                 : ${cyng_obj}")
    message(STATUS "** cyng_store               : ${cyng_store}")
    message(STATUS "** cyng_sys                 : ${cyng_sys}")
    message(STATUS "** cyng_task                : ${cyng_task}")
    message(STATUS "** cyng_vm                  : ${cyng_vm}")
    message(STATUS "** cyng_sqlite3             : ${cyng_sqlite3}")
else()
    message(FATAL_ERROR "** CYNG library not found")
endif()

#
# find crypto library
# -DCRYPT_ROOT:path=...
#
find_package(CRYPTO REQUIRED)
if (CRYPTO_FOUND)
    message(STATUS "** CRYPTO_INCLUDE_DIRS      : ${CRYPTO_INCLUDE_DIRS}")
    message(STATUS "** CRYPTO_LIBRARIES         : ${CRYPTO_LIBRARIES}")
else()
    message(FATAL_ERROR "** CYRPTO library not found")
endif()

#
#   convert to false/true that can be used in C/C++ code
#
string(TOLOWER ${CMAKE_CROSSCOMPILING} ${PROJECT_NAME}_CROSSCOMPILING)

#   use shared libraries
set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build SHARED libraries")

# Find packages ...

# Find Boost
set(Boost_NO_BOOST_CMAKE OFF)
find_package(Boost 1.75 
    REQUIRED 
    COMPONENTS regex iostreams program_options unit_test_framework system thread random date_time
)
# note: https://github.com/chriskohlhoff/asio/issues/585
add_compile_definitions(BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE BOOST_BIND_NO_PLACEHOLDERS BOOST_SPIRIT_UNICODE BOOST_MPL_LIMIT_LIST_SIZE=50)

message(STATUS "** Boost Version                : ${Boost_VERSION}")
message(STATUS "** Boost Include                : ${Boost_INCLUDE_DIRS}")
message(STATUS "** Boost Path                   : ${Boost_LIBRARY_DIRS}")
	
message(STATUS "** Boost_LIBRARIES              : ${Boost_LIBRARIES}")
message(STATUS "** Boost_THREAD_LIBRARY         : ${Boost_THREAD_LIBRARY}")
message(STATUS "** Boost_SYSTEM_LIBRARY         : ${Boost_SYSTEM_LIBRARY}")
message(STATUS "** Boost_DATE_TIME_LIBRARY      : ${Boost_DATE_TIME_LIBRARY}")
message(STATUS "** Boost_PROGRAM_OPTIONS_LIBRARY: ${Boost_PROGRAM_OPTIONS_LIBRARY}")
message(STATUS "** Boost_RANDOM_LIBRARY         : ${Boost_RANDOM_LIBRARY}")
message(STATUS "** Boost_REGEX_LIBRARY          : ${Boost_REGEX_LIBRARY}")
message(STATUS "** Boost_UNIT_TEST_FRAMEWORK    : ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}")

message(STATUS "** Search openSSL   : ${OPENSSL_ROOT_DIR}")
find_package(OpenSSL 1.0.2 REQUIRED)
if(OPENSSL_FOUND)

    add_definitions(-D${PROJECT_NAME}_SSL_INSTALLED)

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
    "$<$<PLATFORM_ID:Linux>:CYNG::CYNG>"
    Boost::boost 
)

#
# add library: smf_cluster
#
include (src/lib/cluster/protocol/lib.cmake)
add_library(smf_cluster ${GLOBAL_LIBRARY_TYPE} ${cluster_lib})

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

target_link_libraries(smf_cluster_bus 
    PRIVATE 
        Boost::boost 
)

#
# add library: smf_http
#
include (src/lib/http/lib.cmake)
add_library(smf_http ${GLOBAL_LIBRARY_TYPE} ${http_lib})

target_include_directories(smf_http
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CYNG_INCLUDE_DIRS}
    PRIVATE
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

target_link_libraries(smf_http 
    PRIVATE 
        Boost::boost 
)

#
# add library: smf_obis
#
include (src/lib/obis/lib.cmake)
add_library(smf_obis ${GLOBAL_LIBRARY_TYPE} ${obis_lib})

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

target_include_directories(smf_mbus
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CYNG_INCLUDE_DIRS}
        ${CRYPTO_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
    PRIVATE
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
        Boost::boost 
)

#
# add library: smf_iec
#
include (src/lib/iec/protocol/lib.cmake)
add_library(smf_iec ${GLOBAL_LIBRARY_TYPE} ${iec_lib})

target_include_directories(smf_iec
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CYNG_INCLUDE_DIRS}
        ${CRYPTO_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
    PRIVATE
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

target_include_directories(smf_hci
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CYNG_INCLUDE_DIRS}
        ${CRYPTO_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
    PRIVATE
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
# add library: smf_ipt
#
include (src/lib/ipt/protocol/lib.cmake)
add_library(smf_ipt ${GLOBAL_LIBRARY_TYPE} ${ipt_lib})

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

target_link_libraries(smf_ipt_bus 
    PRIVATE 
        Boost::boost 
)

#
# add library: smf_serial_bus
#
include (src/lib/serial/bus/lib.cmake)
add_library(smf_serial_bus ${GLOBAL_LIBRARY_TYPE} ${serial_bus})

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
# nodes
#
add_subdirectory(src/nodes/broker-iec)
add_subdirectory(src/nodes/broker-wmbus)
add_subdirectory(src/nodes/dash)
add_subdirectory(src/nodes/imega)
add_subdirectory(src/nodes/ipt)
add_subdirectory(src/nodes/lora)
add_subdirectory(src/nodes/main)
add_subdirectory(src/nodes/modem)
add_subdirectory(src/nodes/mqtt)
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

#
# unit tests
#
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
    "${PROJECT_SOURCE_DIR}/include/smf/version.hpp"
)

#
# GeneratING OPKG files requires the OPKG tools (https://git.yoctoproject.org/cgit/cgit.cgi/opkg-utils)
# fakeroot opkg/opkg-tools/opkg-buildpackage
# Install with: opkg --force-space install /tmp/oecp-smf_0.x_armel.ipk
#

if(${PROJECT_NAME}_CROSS_COMPILE)

	string(TOLOWER ${PROJECT_NAME} OECP_NAME)
    message(STATUS "** cross compile project    : ${OECP_NAME}")

	configure_file (
		  "opkg/control.in"
		  "${PROJECT_BINARY_DIR}/opkg/control")

	configure_file (
		  "opkg/postinst.in"
		  "${PROJECT_BINARY_DIR}/opkg/postinst")

	configure_file (
		  "opkg/rules.in"
		  "${PROJECT_BINARY_DIR}/opkg/rules")

endif()

#
# install
#
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/smf" # source directory
        DESTINATION "include" # target directory
        FILES_MATCHING # install only matched files
        PATTERN "*.h"
        PATTERN "*hpp"
)

set(SMF_TARGET_LIST smf_cluster smf_cluster_bus smf_config smf_http smf_ipt smf_ipt_bus smf_obis smf_serial_bus dash imega ipt segw store lora main modem setup mqtt)

message(STATUS "** Target list                  : ${SMF_TARGET_LIST}")

install(TARGETS ${SMF_TARGET_LIST}
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

set(ConfigPackageLocation lib/cmake/smf)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/node/NodeTargets.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/node/ConfigVersion.cmake"
    DESTINATION
        ${ConfigPackageLocation}
    COMPONENT
        Devel
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

    find_program(LSB_RELEASE_EXEC lsb_release)
    execute_process(COMMAND ${LSB_RELEASE_EXEC} -is
        OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "** LSB release                  : ${LSB_RELEASE_ID_SHORT}")

    if (${LSB_RELEASE_ID_SHORT} STREQUAL "openSUSE")
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
