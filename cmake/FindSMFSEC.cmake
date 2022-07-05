# FindSMFSEC.cmake
#
# Finds the SMFSEC library
#
# This will define the following variables
#
#   SMFSEC_FOUND          - system has smfsec
#   SMFSEC_INCLUDE_DIRS    - the smfsec include directories
#   SMFSEC_LIBRARIES      - smfsec libraries directories
#
# and the following imported targets
#
#     smfsec::smfsec
#

message(STATUS "** CYNG Tree Name     : [${SMF_BUILD_TREE_STEM}]")

#
#	try PkgConfig
#
find_package(PkgConfig)
pkg_check_modules(PC_SMFSEC QUIET SMFSEC)

if(PC_SMFSEC_FOUND)
    # the library was already found by pkgconfig
    set(SMFSEC_VERSION ${PC_SMFSEC_VERSION})
    set(SMFSEC_INCLUDE_DIRS ${PC_SMFSEC_INCLUDE_DIRS})
    set(SMFSEC_LIBRARIES ${PC_SMFSEC_LIBRARIES})
     
    if(NOT PC_SMFSEC_QUIETLY)
        message(STATUS "** SMFSEC_PKG_FOUND: BEGIN")
        message(STATUS "** SMFSEC_VERSION: ${SMFSEC_VERSION}")
        message(STATUS "** SMFSEC_INCLUDE_DIRS: ${SMFSEC_INCLUDE_DIRS}")
        message(STATUS "** SMFSEC_LIBRARIES: ${SMFSEC_LIBRARIES}")
        message(STATUS "** SMFSEC_PKG_FOUND: END")
    endif(NOT PC_SMFSEC_QUIETLY)

else(PC_SMFSEC_FOUND)

	#
	#
	file(GLOB SMFSEC_SEARCH_PATH "${CMAKE_PREFIX_PATH}/crypto*" "${CMAKE_PREFIX_PATH}/smfsec*" "${CMAKE_PREFIX_PATH}/libsmfsec*" "${PROJECT_SOURCE_DIR}/../crypto*" "${PROJECT_SOURCE_DIR}/../smfsec*" "${PROJECT_SOURCE_DIR}/../libsmfsec*" "${PROJECT_SOURCE_DIR}/../../sysroot-target")
    find_path(SMFSEC_INCLUDE_DIR_SRC
        NAMES 
            smfsec/crypto.h
			smfsec/bio.h
        PATH_SUFFIXES
            include
        PATHS
            ${SMFSEC_SEARCH_PATH}
        DOC 
            "SMFSEC headers"
		NO_CMAKE_FIND_ROOT_PATH
    )
    
	
	#
	#	search smfsec libraries
	#	On Windows the Debug build is preferred.
	#
    set(REQUESTED_LIBS "smfsec")

	foreach(__LIB ${REQUESTED_LIBS})
		find_library("${__LIB}" ${__LIB}
            PATHS
                ${SMFSEC_SEARCH_PATH}
            PATH_SUFFIXES
				"lib"
                "usr/lib/"
				"${SMF_BUILD_TREE_STEM}"
				"${SMF_BUILD_TREE_STEM}/Debug"
				"${SMF_BUILD_TREE_STEM}/Release"
			DOC 
				"SMFSEC libraries"
		)

		# append the found library to the list of all libraries
		list(APPEND SMFSEC_LIBRARIES ${${__LIB}})

		# this creates a variable with the name of the searched library, so that it can be included more easily
		unset(__LIB_UPPERCASE_NAME)
		string(TOUPPER ${__LIB} __LIB_UPPERCASE_NAME)
		set(${__LIB_UPPERCASE_NAME}_LIBRARY ${${__LIB}})
	endforeach()
endif(PC_SMFSEC_FOUND)
    
# check if both the header and the libraries have been found
if (SMFSEC_INCLUDE_DIR_SRC AND SMFSEC_LIBRARIES)
    set(SMFSEC_INCLUDE_DIRS "${SMFSEC_INCLUDE_DIR_SRC}")
    set(SMFSEC_FOUND ON)
    unset(SMFSEC_INCLUDE_DIR_SRC CACHE)
	if(NOT SMFSEC_FIND_QUIETLY)
		message(STATUS "** SMFSEC_LIBRARIES         : ${SMFSEC_LIBRARIES}")
		message(STATUS "** SMFSEC_INCLUDE_DIRS      : ${SMFSEC_INCLUDE_DIRS}")
	endif(NOT SMFSEC_FIND_QUIETLY)
endif(SMFSEC_INCLUDE_DIR_SRC AND SMFSEC_LIBRARIES)

mark_as_advanced(SMFSEC_FOUND SMFSEC_INCLUDE_DIRS SMFSEC_LIBRARIES)

if(UNIX)
	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(SMFSEC
		REQUIRED_VARS 
			SMFSEC_INCLUDE_DIRS
			SMFSEC_LIBRARIES
		VERSION_VAR 
			SMFSEC_VERSION
		FAIL_MESSAGE
			"Cannot provide SMFSEC library"
	)
endif(UNIX)

if(SMFSEC_FOUND AND NOT TARGET smfsec::smfsec)

    add_library(smfsec::smfsec INTERFACE IMPORTED)

#	define a target
   	set_target_properties(smfsec::smfsec 
		PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES 
				"${SMFSEC_INCLUDE_DIRS}"
			INTERFACE_LINK_LIBRARIES 
				"${SMFSEC_LIBRARIES}"
    )

endif(SMFSEC_FOUND AND NOT TARGET smfsec::smfsec)