# FindCYNG.cmake
#
# Finds the CYNG library
#
# This will define the following variables
#
#   CYNG_FOUND          - system has cyng
#   CYNG_INCLUDE_DIRS   - the cyng include directories
#   CYNG_LIBRARIES      - cyng libraries directories
#
# and the following imported targets
#
#     cyng::cyng
#

#
#	try PkgConfig
#
find_package(PkgConfig)
pkg_check_modules(PC_CYNG QUIET CYNG)

if(PC_CYNG_FOUND)
    # the library was already found by pkgconfig
    set(CYNG_VERSION ${PC_CYNG_VERSION})
    set(CYNG_INCLUDE_DIRS ${PC_CYNG_INCLUDE_DIRS})
    set(CYNG_LIBRARIES ${PC_CYNG_LIBRARIES})
     
    if(NOT PC_CYNG_QUIETLY)
        message(STATUS "** CYNG_PKG_FOUND: BEGIN")
        message(STATUS "** CYNG_VERSION: ${CYNG_VERSION}")
        message(STATUS "** CYNG_INCLUDE_DIRS: ${CYNG_INCLUDE_DIRS}")
        message(STATUS "** CYNG_LIBRARIES: ${CYNG_LIBRARIES}")
        message(STATUS "** CYNG_PKG_FOUND: END")
    endif(NOT PC_CYNG_QUIETLY)

else(PC_CYNG_FOUND)

	#
	#	cyng header files, which are included in the form cyng/xx.h
	#
	file(GLOB CYNG_SEARCH_PATH "${CMAKE_PREFIX_PATH}/cyng*" "${PROJECT_SOURCE_DIR}/../cyng*" "${PROJECT_SOURCE_DIR}/../../sysroot-target")
    find_path(CYNG_INCLUDE_DIR_SRC
        NAMES 
            cyng/version.hpp
			cyng/meta.hpp
        PATH_SUFFIXES
            include
        PATHS
            ${CYNG_SEARCH_PATH}
        DOC 
            "CYNG headers"
		NO_CMAKE_FIND_ROOT_PATH
    )
    
	#
	#	cyng generated header files
	#
    find_path(CYNG_INCLUDE_DIR_BUILD
        NAMES 
            cyng.h
        PATH_SUFFIXES
			include
			build/v5te/include
			build/x64/include
			build/include
			v5te/include
        PATHS
			${CYNG_SEARCH_PATH}
        DOC 
            "CYNG headers"
		NO_CMAKE_FIND_ROOT_PATH
    )

	#
	#	Search cyng libraries.
    #   In a multiplatform environment the OECP(1) builds are located in the "v5te" directory
    #   and this directory is preferred.
	#	On Windows the Debug build is preferred.
	#
    set(REQUESTED_LIBS "cyng_db;cyng_io;cyng_log;cyng_obj;cyng_parse;cyng_rnd;cyng_sql;cyng_store;cyng_sys;cyng_task;cyng_vm;cyng_net;cyng_sqlite3")
    
	if(WIN32)
        list(APPEND REQUESTED_LIBS "cyng_scm")
    endif(WIN32)

	foreach(__LIB ${REQUESTED_LIBS})
		find_library("${__LIB}" ${__LIB}
            PATHS
                ${CYNG_SEARCH_PATH}
            PATH_SUFFIXES
				"lib"
                "usr/lib/"
				"build/v5te"
				"build/x64"
                "v5te/src/net"
				"build/src/net"
				"build"
				"build/Debug"
				"build/Release"
				"v5te"
				"src/net"
				"build/src/net/Debug"
				"build/src/net/Release"
			DOC 
				"CYNG libraries"
		)

		# append the found library to the list of all libraries
		list(APPEND CYNG_LIBRARIES ${${__LIB}})

		# this creates a variable with the name of the searched library, so that it can be included more easily
		unset(__LIB_UPPERCASE_NAME)
		string(TOUPPER ${__LIB} __LIB_UPPERCASE_NAME)
		set(${__LIB_UPPERCASE_NAME}_LIBRARY ${${__LIB}})
	endforeach()
endif(PC_CYNG_FOUND)
    
# check if both the header and the libraries have been found
if (CYNG_INCLUDE_DIR_SRC AND CYNG_INCLUDE_DIR_BUILD AND CYNG_LIBRARIES)
    set(CYNG_INCLUDE_DIRS "${CYNG_INCLUDE_DIR_SRC};${CYNG_INCLUDE_DIR_BUILD}")
    set(CYNG_FOUND ON)
    unset(CYNG_INCLUDE_DIR_SRC CACHE)
	unset(CYNG_INCLUDE_DIR_BUILD CACHE)
	if(NOT CYNG_FIND_QUIETLY)
		message(STATUS "** CYNG_LIBRARIES    : ${CYNG_LIBRARIES}")
		message(STATUS "** CYNG_INCLUDE_DIRS : ${CYNG_INCLUDE_DIRS}")
	endif(NOT CYNG_FIND_QUIETLY)
endif(CYNG_INCLUDE_DIR_SRC AND CYNG_INCLUDE_DIR_BUILD AND CYNG_LIBRARIES)

mark_as_advanced(CYNG_FOUND CYNG_INCLUDE_DIRS CYNG_LIBRARIES)

if(UNIX)
	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(CYNG
		REQUIRED_VARS 
			CYNG_INCLUDE_DIRS
			CYNG_LIBRARIES
		VERSION_VAR 
			CYNG_VERSION
		FAIL_MESSAGE
			"Cannot provide CYNG library"
	)
endif(UNIX)

if(CYNG_FOUND AND NOT TARGET cyng::cyng)

    add_library(cyng::cyng INTERFACE IMPORTED)

#	define a target
   	set_target_properties(cyng::cyng 
		PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES 
				"${CYNG_INCLUDE_DIRS}"
			INTERFACE_LINK_LIBRARIES 
				"${CYNG_LIBRARIES}"
    )

endif(CYNG_FOUND AND NOT TARGET cyng::cyng)