#
#
cmake_minimum_required (VERSION 3.5)

                                                     
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
# set project name/properties
#
project(smf
	VERSION 0.9.0.1
	LANGUAGES CXX C
)


# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    message(STATUS "** Compiler           : ${CMAKE_CXX_COMPILER_ID} v${CMAKE_CXX_COMPILER_VERSION}")

    # Let's ensure -std=c++xx instead of -std=g++xx
    set(CMAKE_CXX_EXTENSIONS OFF)

    set(CPACK_PROJECT_NAME ${PROJECT_NAME})
    set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
    include(CPack)


    # Docs only available if this is the main app
    find_package(Doxygen)
    if(Doxygen_FOUND)
        add_subdirectory(docs)
    else()
        message(STATUS "Doxygen not found, not building docs")
    endif()

    if (WIN32)
        message(STATUS "Use static libraries")
        set(GLOBAL_LIBRARY_TYPE STATIC)
        #set(Boost_USE_STATIC_LIBS ON)
        # target is Windows 10
	    add_compile_definitions(_WIN32_WINNT=0x0A00 _SCL_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
        add_compile_options("/bigobj")
        message(STATUS "** compile definitions: ${CMAKE_CXX_FLAGS}")

    else()
        set(GLOBAL_LIBRARY_TYPE SHARED)
    endif()

    #
    #   test endian
    #
    include (TestBigEndian)
    TEST_BIG_ENDIAN(${PROJECT_NAME}_BIG_ENDIAN)
    message(STATUS "** big endian         : ${${PROJECT_NAME}_BIG_ENDIAN}")

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
    message(STATUS "** cyng_io                  : ${cyng_io}")
    message(STATUS "** cyng_log                 : ${cyng_log}")
    message(STATUS "** cyng_obj                 : ${cyng_obj}")
    message(STATUS "** cyng_store               : ${cyng_store}")
    message(STATUS "** cyng_sys                 : ${cyng_sys}")
    message(STATUS "** cyng_task                : ${cyng_task}")
    message(STATUS "** cyng_vm                  : ${cyng_vm}")
else()
    message(FATAL_ERROR "** cyng library not found")
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

message(STATUS "** Boost Version    : ${Boost_VERSION}")
message(STATUS "** Boost Include    : ${Boost_INCLUDE_DIRS}")
message(STATUS "** Boost Path       : ${Boost_LIBRARY_DIRS}")
	
message(STATUS "** Boost_LIBRARIES              : ${Boost_LIBRARIES}")
message(STATUS "** Boost_THREAD_LIBRARY         : ${Boost_THREAD_LIBRARY}")
message(STATUS "** Boost_SYSTEM_LIBRARY         : ${Boost_SYSTEM_LIBRARY}")
message(STATUS "** Boost_DATE_TIME_LIBRARY      : ${Boost_DATE_TIME_LIBRARY}")
message(STATUS "** Boost_PROGRAM_OPTIONS_LIBRARY: ${Boost_PROGRAM_OPTIONS_LIBRARY}")
message(STATUS "** Boost_RANDOM_LIBRARY         : ${Boost_RANDOM_LIBRARY}")
message(STATUS "** Boost_REGEX_LIBRARY          : ${Boost_REGEX_LIBRARY}")
message(STATUS "** Boost_UNIT_TEST_FRAMEWORK    : ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}")


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
        Boost::boost 
)

#
# add library: smf_cluster
#
include (src/lib/cluster/lib.cmake)
add_library(smf_cluster ${GLOBAL_LIBRARY_TYPE} ${cluster_lib})

target_include_directories(smf_cluster
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
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
# nodes
#
add_subdirectory(src/nodes/frontend)
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
# unit tests
#
#add_subdirectory(unit_test)

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

