# FindCYNG.cmake
#
# Finds the CYNG library
#
# This will define the following variables
#
#   CYNG_FOUND          - system has cyng
#   CYNG_INCLUDE_DIR    - the cyng include directories
#   CYNG_LIBRARIES      - cyng libraries directories
#
# and the following imported targets
#
#     CYNG::CYNG
#

set(__CYNG_DEVELOP_ROOT "${PROJECT_SOURCE_DIR}/../cyng" CACHE PATH "__CYNG_DEVELOP_ROOT")
get_filename_component(CYNG_DEVELOP_ROOT ${__CYNG_DEVELOP_ROOT} REALPATH)
unset(__CYNG_DEVELOP_ROOT)

#
#	check path definition for CYNG development root directory
#
if(NOT DEFINED CYNG_ROOT_DEV)
	set(CYNG_ROOT_DEV ${CYNG_DEVELOP_ROOT} CACHE PATH "cyng develop root directory")
endif()
message(STATUS "** CYNG root d     : ${CYNG_ROOT_DEV}")

#
#	check path definition for CYNG build directory
#
if(NOT DEFINED CYNG_ROOT_BUILD)
	set(CYNG_ROOT_BUILD "${CYNG_ROOT_BUILD}/build" CACHE PATH "CYNG_ROOT_BUILD")
endif()
message(STATUS "** CYNG root build : ${CYNG_ROOT_BUILD}")

find_package(PkgConfig)
pkg_check_modules(PC_CYNG QUIET CYNG)

if(CYNG_PKG_FOUND)

    set(CYNG_VERSION ${PC_CYNG_VERSION})
    set(CYNG_INCLUDE_DIRS ${CYNG_PKG_INCLUDE_DIRS})
    set(CYNG_LIBRARIES ${CYNG_PKG_LIBRARIES})

else(CYNG_PKG)

	#
	#	cyng header files
	#
    find_path(CYNG_INCLUDE_DIR_1
        NAMES 
            cyng/cyng.h
            cyng/object.h
        PATH_SUFFIXES
            cyng
        HINTS
            "${CYNG_ROOT_DEV}/src/main/include"
        PATHS
            /usr/include/
            /usr/local/include/
        DOC 
            "CYNG headers"
    )
    
#   message(STATUS "** found 1: ${CYNG_INCLUDE_DIR_1}")

	#
	#	cyng generated header files
	#
   find_path(CYNG_INCLUDE_DIR_2
        NAMES 
            CYNG_project_info.h
         HINTS
            "${CYNG_ROOT_DEV}/build"
            "${CYNG_ROOT_DEV}/build/x64"
            /usr/include/
            /usr/local/include/
        DOC 
            "CYNG headers"
    )

	#
	#	cyng submodule header files
	#
   find_path(CYNG_INCLUDE_DIR_3
        NAMES 
            pugixml.hpp
         HINTS
            "${CYNG_ROOT_DEV}/lib/xml/pugixml/src"
            /usr/include/
            /usr/local/include/
        DOC 
            "pugixml headers"
    )

#    message(STATUS "** found 2: ${CYNG_INCLUDE_DIR_2}")
    # libcyng_async.so   libcyng_csv.so        libcyng_domain.so  libcyng_log.so   libcyng_parser.so  libcyng_store.so  libcyng_vm.so
    # libcyng_core.so    libcyng_db.so         libcyng_io.so      libcyng_mail.so  libcyng_rnd.so     libcyng_sys.so    libcyng_xml.so
    # libcyng_json.so    libcyng_meta.so       libcyng_sql.so     libcyng_table.so

    set(CYNG_LIB_LIST "cyng_async;cyng_csv;cyng_domain;cyng_log;cyng_parser;cyng_store;cyng_vm;cyng_core;cyng_db;cyng_io;cyng_crypto;cyng_mail;cyng_rnd;cyng_sys;cyng_xml;cyng_json;cyng_meta;cyng_sql;cyng_table;cyng_sqlite3")
	if(WIN32)
		list(APPEND CYNG_LIB_LIST "cyng_scm")
	endif()

    if (UNIX)
#        set(__CYNG_BUILD "${CYNG_ROOT_DEV}/build" CACHE PATH "__CYNG_BUILD")

		foreach(CYNG_LIB ${CYNG_LIB_LIST})
			find_library("__${CYNG_LIB}" ${CYNG_LIB}
				HINTS
					${CYNG_ROOT_BUILD}
				PATHS
					/usr/lib/
					/usr/local/lib
				DOC 
					"CYNG libraries"
			)
        
			list(APPEND CYNG_LIBRARIES ${__${CYNG_LIB}})
			#message(STATUS "** __CYNG_LIB    : ${__${CYNG_LIB}}")
		endforeach()

    else()
        #
        #	$(ConfigurationName) is a variable used by the MS build system
        #
        set(__CYNG_BUILD_OPTIMIZED "${CYNG_ROOT_BUILD}/Release" CACHE PATH "__CYNG_BUILD_OPTIMIZED")
        set(__CYNG_BUILD_DEBUG "${CYNG_ROOT_BUILD}/Debug" CACHE PATH "__CYNG_BUILD_DEBUG")

		foreach(CYNG_LIB ${CYNG_LIB_LIST})

		#
		#	optimized library
		#
			list(APPEND CYNG_LIBRARIES "optimized")
			list(APPEND CYNG_LIBRARIES "${__CYNG_BUILD_OPTIMIZED}/${CYNG_LIB}.lib")

		#
		#	debug library
		#
			list(APPEND CYNG_LIBRARIES "debug")
			list(APPEND CYNG_LIBRARIES "${__CYNG_BUILD_DEBUG}/${CYNG_LIB}.lib")

			#message(STATUS "** CYNG_LIBRARIES    : ${CYNG_LIBRARIES}")

			#set(${CYNG_LIB} "optimized;${__CYNG_BUILD_OPTIMIZED}/${CYNG_LIB}.lib;debug;${__CYNG_BUILD_DEBUG}/${CYNG_LIB}.lib")
			#message(STATUS "** CYNG_LIB    : ${CYNG_LIB}")

		endforeach()

    endif(UNIX)
    
    
    #
    #  remove the cyng prefix - maybe there is a better solution
    #
    if (UNIX)
		get_filename_component(CYNG_INCLUDE_DIR_1 "${CYNG_INCLUDE_DIR_1}/../" REALPATH)
	endif()
	set(CYNG_INCLUDE_DIRS "${CYNG_INCLUDE_DIR_1};${CYNG_INCLUDE_DIR_2};${CYNG_INCLUDE_DIR_3}")
	unset(CYNG_INCLUDE_DIR_1)
	unset(CYNG_INCLUDE_DIR_2)
	unset(CYNG_INCLUDE_DIR_3)
    
    get_filename_component(CYNG_MODULE_PATH "${CYNG_ROOT_DEV}/src/modules/" REALPATH)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CYNG_MODULE_PATH}")

	if (CYNG_INCLUDE_DIRS AND CYNG_LIBRARIES)
		set(CYNG_FOUND ON)
	endif()
    
endif(CYNG_PKG_FOUND)


mark_as_advanced(CYNG_FOUND CYNG_INCLUDE_DIRS CYNG_LIBRARIES)

if(UNIX)
	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(CYNG
		REQUIRED_VARS 
			CYNG_INCLUDE_DIRS
			CYNG_LIBRARIES
		VERSION_VAR 
			CYNG_VERSION
	)
endif(UNIX)

# if(CYNG_FOUND AND NOT TARGET CYNG::CYNG)
#     add_library(CYNG::CYNG INTERFACE IMPORTED)
#     set_target_properties(CYNG::CYNG PROPERTIES
#         INTERFACE_INCLUDE_DIRECTORIES 
#             ${CYNG_INCLUDE_DIRS}
#          INTERFACE_LINK_LIBRARIES 
#              ${CYNG_LIBRARIES}
#     )
# 
# endif()
