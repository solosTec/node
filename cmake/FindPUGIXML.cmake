# FindPUGIXML.cmake
#
# Finds the PUGIXML library
#
# This will define the following variables
#
#   PUGIXML_FOUND          - system has pugixml
#   PUGIXML_INCLUDE_DIRS	- the pugixml include directories
#   PUGIXML_LIBRARIES      - pugixml libraries directories
#	<NAME>_LIBRARY			- individual variable for each library found
#
# and the following imported targets
#
#     pugixml::pugixml
#

# clear the variables, in case they were set somewhere elese
if(PUGIXML_LIBRARIES)
	unset(PUGIXML_LIBRARIES)
	unset(PUGIXML_LIBRARIES CACHE)
	unset(PUGIXML_LIBRARIES PARENT_SCOPE)
endif()

if(PUGIXML_INCLUDE_DIRS)
	unset(PUGIXML_INCLUDE_DIRS)
	unset(PUGIXML_INCLUDE_DIRS CACHE)
	unset(PUGIXML_INCLUDE_DIRS PARENT_SCOPE)
endif()

#
#	try PkgConfig
#
find_package(PkgConfig)
pkg_check_modules(PC_PUGIXML QUIET PUGIXML)

if(PC_PUGIXML_FOUND)
    set(PUGIXML_VERSION ${PC_PUGIXML_VERSION})
    set(PUGIXML_INCLUDE_DIRS ${PC_PUGIXML_INCLUDE_DIRS})
    set(PUGIXML_LIBRARIES ${PC_PUGIXML_LIBRARIES})

	if(NOT PC_PUGIXMLQUIETLY)
		message(STATUS "** PC_PUGIXML_FOUND: BEGIN")
		message(STATUS "** PUGIXML_VERSION: ${PUGIXML_VERSION}")
		message(STATUS "** PUGIXML_INCLUDE_DIRS: ${PUGIXML_INCLUDE_DIRS}")
		message(STATUS "** PUGIXML_LIBRARIES: ${PUGIXML_LIBRARIES}")
		message(STATUS "** PC_PUGIXML_FOUND: END")
	endif(NOT PC_PUGIXML_QUIETLY)

else(PC_PUGIXML_FOUND)

	#
	#	pugixml header files
	#
	file(GLOB PUGIXML_SEARCH_PATH 
		"${CMAKE_PREFIX_PATH}/pugixml*" 
		"${PROJECT_SOURCE_DIR}/../pugixml*" 
		"${PROJECT_SOURCE_DIR}/../../sysroot-target" 
        "${PROJECT_SOURCE_DIR}/../../packages/pugixml*"
		"${PROJECT_SOURCE_DIR}/../../root")
    find_path(PUGIXML_INCLUDE_DIRS
        NAMES 
            pugixml.hpp 
            pugiconfig.hpp
		PATH_SUFFIXES
			usr/include
			include
        PATHS
			${PUGIXML_SEARCH_PATH}
        DOC 
            "PUGIXML headers"
		NO_CMAKE_FIND_ROOT_PATH
    )

    #
	#	search pugixml libraries on linux
	#
	set(FIND_LIBS "pugixml")

	foreach(__LIB ${FIND_LIBS})
		find_library("${__LIB}" ${__LIB}
			PATHS
				${PUGIXML_SEARCH_PATH}
			PATH_SUFFIXES
				usr/lib
				lib
				build
			DOC 
				"PUGIXML libraries"
            NO_CMAKE_FIND_ROOT_PATH
		)
		
		# append the found library to the list of all libraries
		list(APPEND PUGIXML_LIBRARIES ${${__LIB}})

		# this creates a variable with the name of the searched library, so that it can be included more easily
		unset(__LIB_UPPERCASE_NAME)
		string(TOUPPER ${__LIB} __LIB_UPPERCASE_NAME)
		set(${__LIB_UPPERCASE_NAME}_LIBRARY ${${__LIB}})
	endforeach()

endif(PC_PUGIXML_FOUND)

# check if both the header and the libraries have been found
if (PUGIXML_INCLUDE_DIRS AND PUGIXML_LIBRARIES)
	set(PUGIXML_FOUND ON)
	if(NOT PUGIXML_FIND_QUIETLY)
		message(STATUS "** PUGIXML_LIBRARIES    : ${PUGIXML_LIBRARIES}")
		message(STATUS "** PUGIXML_INCLUDE_DIRS : ${PUGIXML_INCLUDE_DIRS}")
	endif(NOT PUGIXML_FIND_QUIETLY)
endif(PUGIXML_INCLUDE_DIRS AND PUGIXML_LIBRARIES)

mark_as_advanced(PUGIXML_FOUND PUGIXML_INCLUDE_DIRS PUGIXML_LIBRARIES)

if(UNIX)
	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(PUGIXML
		REQUIRED_VARS 
			PUGIXML_INCLUDE_DIRS
			PUGIXML_LIBRARIES
		VERSION_VAR 
			PUGIXML_VERSION
		FAIL_MESSAGE
			"Cannot provide PUGIXML library"
	)
endif(UNIX)

if(PUGIXML_FOUND AND NOT TARGET pugixml::pugixml)

    add_library(pugixml::pugixml INTERFACE IMPORTED)

#	define a target
   	set_target_properties(pugixml::pugixml 
		PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES 
				"${PUGIXML_INCLUDE_DIRS}"
			INTERFACE_LINK_LIBRARIES 
				"${PUGIXML_LIBRARIES}"
    )

endif()

