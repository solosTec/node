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
#   cyng::cyng
#	cyng::db
#	cyng::io
#	cyng::log
#	cyng::obj
#	cyng::parse
#	cyng::rnd
#	cyng::sql
#	cyng::store
#	cyng::sys
#	cyng::task
#	cyng::vm
#	cyng::net
#	cyng::sqlite3
#

message(STATUS "** CYNG Tree Name     : [${SMF_BUILD_TREE_STEM}]")

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
			${SMF_BUILD_TREE_STEM}/include
			build/include
        PATHS
			${CYNG_SEARCH_PATH}
        DOC 
            "CYNG headers"
		NO_CMAKE_FIND_ROOT_PATH
    )

	if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20") 
		cmake_path(GET CYNG_INCLUDE_DIR_BUILD PARENT_PATH CYNG_BUILD_DIR)
	else()
		get_filename_component(CYNG_BUILD_DIR ${CYNG_INCLUDE_DIR_BUILD} DIRECTORY)
	endif()
	message(STATUS "** CYNG_BUILD_DIR           : ${CYNG_BUILD_DIR}")

    find_file(CYNG_PUGI_XML_LIB
        NAMES 
            pugixml.lib
			libpugixml.so
        PATH_SUFFIXES
			"_deps/pugixml-build"
			"_deps/pugixml-build/Debug"
			"_deps/pugixml-build/Release"
        PATHS
			${CYNG_BUILD_DIR}
        DOC 
            "PugiXML"
		NO_CMAKE_FIND_ROOT_PATH
    )
	message(STATUS "** CYNG_PUGI_XML_LIB        : ${CYNG_PUGI_XML_LIB}")

	#
	#	Search cyng libraries.
    #   In a multiplatform environment the OECP(1) builds are located in the "v5te" directory
    #   and this directory is preferred.
	#	On Windows the Debug build is preferred.
	#
    set(REQUESTED_LIBS "db;io;log;obj;parse;rnd;sql;store;sys;task;vm;net;sqlite3")
    
	if(WIN32)
        list(APPEND REQUESTED_LIBS "scm")
    endif(WIN32)

	foreach(__LIB ${REQUESTED_LIBS})

		find_library("__CYNG_${__LIB}" 
			NAME
				"cyng_${__LIB}"
            PATHS
                ${CYNG_SEARCH_PATH}
            PATH_SUFFIXES
				"lib"
                "usr/lib/"
				"${SMF_BUILD_TREE_STEM}"
                "${SMF_BUILD_TREE_STEM}/src/net"
				"build/src/net"
				"build"
				"build/Debug"
				"build/Release"
			DOC 
				"CYNG libraries"
		)

#		message(STATUS "** add library cyng::${__LIB}: ${__CYNG_${__LIB}}")
		add_library("cyng::${__LIB}" INTERFACE IMPORTED)
		set_target_properties("cyng::${__LIB}" 
				PROPERTIES
					INTERFACE_INCLUDE_DIRECTORIES 
						"${CYNG_INCLUDE_DIRS}"
					INTERFACE_LINK_LIBRARIES 
						"${__CYNG_${__LIB}}"
			)

		# append the found library to the list of all libraries
		list(APPEND CYNG_LIBRARIES ${__CYNG_${__LIB}})

		# this creates a variable with the name of the searched library, so that it can be included more easily
		#unset(__LIB_UPPERCASE_NAME)
		string(TOUPPER "cyng_${__LIB}" __LIB_UPPERCASE_NAME)
#		message(STATUS "** lib name  : ${__LIB_UPPERCASE_NAME}: ${__CYNG_${__LIB}}")
		set(${__LIB_UPPERCASE_NAME} ${__CYNG_${__LIB}})
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