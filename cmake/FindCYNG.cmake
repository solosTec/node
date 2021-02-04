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

#set(__CYNG_DEVELOP_ROOT "${PROJECT_SOURCE_DIR}/../cyng" CACHE PATH "__CYNG_DEVELOP_ROOT")
#get_filename_component(CYNG_DEVELOP_ROOT ${__CYNG_DEVELOP_ROOT} REALPATH)
#unset(__CYNG_DEVELOP_ROOT CACHE)

#
#	check path definition for CYNG development root directory
#
#if(NOT DEFINED CYNG_ROOT_DEV)
#	set(CYNG_ROOT_DEV ${CYNG_DEVELOP_ROOT} CACHE PATH "cyng develop root directory")
#endif()
#message(STATUS "** CYNG root dev   : ${CYNG_ROOT_DEV}")

#
#	check path definition for CYNG build directory
#
#if(NOT DEFINED CYNG_ROOT_BUILD_SUBDIR)
#	set(CYNG_ROOT_BUILD_SUBDIR "build" CACHE STRING "CYNG_ROOT_BUILD_SUBDIR")
#endif()
#message(STATUS "** CYNG root build : ${CYNG_ROOT_BUILD_SUBDIR}")



#
#	try PkgConfig
#
find_package(PkgConfig)
pkg_check_modules(PC_CYNG QUIET CYNG)

if(CYNG_PKG_FOUND)

    set(CYNG_VERSION ${PC_CYNG_VERSION})
    set(CYNG_INCLUDE_DIRS ${CYNG_PKG_INCLUDE_DIRS})
    set(CYNG_LIBRARIES ${CYNG_PKG_LIBRARIES})
       
endif(CYNG_PKG_FOUND)

set(CYNG_LIBS "cyng_io;cyng_log;cyng_obj;cyng_parse;cyng_rnd;cyng_sql;cyng_store;cyng_sys;cyng_task;cyng_vm")
#if(WIN32)
#	list(APPEND CYNG_LIB_LIST "cyng_scm")
#endif()

if(NOT CYNG_FOUND)

	#
	#	cyng header files
	#
    find_path(CYNG_INCLUDE_DIR_SRC
        NAMES 
            cyng/version.hpp
            cyng/obj/object.h
        PATH_SUFFIXES
            cyng
        HINTS
            "${PROJECT_SOURCE_DIR}/../cyng/include"
        PATHS
            /usr/include/
            /usr/local/include/
        DOC 
            "CYNG headers"
    )
    
	message(STATUS "** CYNG_INCLUDE_DIR_SRC: ${CYNG_INCLUDE_DIR_SRC}")

	#
	#	cyng generated header files
	#
   find_path(CYNG_INCLUDE_DIR_BUILD
        NAMES 
            cyng.h
         HINTS
			"${PROJECT_SOURCE_DIR}/../cyng/build/include"
            "${PROJECT_SOURCE_DIR}/../cyng/build/x64/include"
            "${PROJECT_SOURCE_DIR}/../cyng/build/v5te/include"
        PATHS
            /usr/include/
            /usr/local/include/
        DOC 
            "CYNG headers"
    )

	message(STATUS "** CYNG_INCLUDE_DIR_BUILD: ${CYNG_INCLUDE_DIR_BUILD}")

	if (WIN32)

	#
	#	search cyng libraries on windows
	#
	foreach(__CYNG_LIB ${CYNG_LIBS})

		message(STATUS "** hint : ${__CYNG_LIB}")

		find_library("${__CYNG_LIB}" ${__CYNG_LIB}
			NAMES
				${__CYNG_BUILD}
			HINTS
				"${CYNG_INCLUDE_DIR_BUILD}/../Debug"
				"${CYNG_INCLUDE_DIR_BUILD}/../Release"
			PATHS
				/usr/lib/
				/usr/local/lib
			DOC 
				"CYNG libraries"
		)
		message(STATUS "** found : ${${__CYNG_LIB}}")
		list(APPEND CYNG_LIBRARIES ${${__CYNG_LIB}})
#		message(STATUS "** CYNG_LIBRARIES    : ${CYNG_LIBRARIES}")

	endforeach()

	else(UNIX)

	#
	#	search cyng libraries on linux
	#
	foreach(CYNG_LIB ${CYNG_LIBS})

		message(STATUS "** hint : ${CYNG_LIB}")

	endforeach()

	endif()

#    if (UNIX)

# 		set(__CYNG_BUILD "${CYNG_ROOT_DEV}/${CYNG_ROOT_BUILD_SUBDIR}" CACHE PATH "__CYNG_BUILD")
#		foreach(CYNG_LIB ${CYNG_LIB_LIST})
#			find_library("__${CYNG_LIB}" ${CYNG_LIB}
#				HINTS
#					${__CYNG_BUILD}
#				PATHS
#					/usr/lib/
#					/usr/local/lib
#				DOC 
#					"CYNG libraries"
#			)
        
#			list(APPEND CYNG_LIBRARIES ${__${CYNG_LIB}})
			#message(STATUS "** __CYNG_LIB    : ${__${CYNG_LIB}}")
			
			#
			# ToDo: define some variables for install script
			#
#		endforeach()

 #   else()
        #
        #	$(ConfigurationName) is a variable used by the MS build system
        #
 		#set(__CYNG_BUILD_OPTIMIZED "${CYNG_ROOT_DEV}/${CYNG_ROOT_BUILD_SUBDIR}/Release" CACHE PATH "__CYNG_BUILD_OPTIMIZED")
		#set(__CYNG_BUILD_DEBUG "${CYNG_ROOT_DEV}/${CYNG_ROOT_BUILD_SUBDIR}/Debug" CACHE PATH "__CYNG_BUILD_DEBUG")

#		foreach(CYNG_LIB ${CYNG_LIB_LIST})

		#
		#	optimized library
		#
		#	list(APPEND CYNG_LIBRARIES "optimized")
		#	list(APPEND CYNG_LIBRARIES "${__CYNG_BUILD_OPTIMIZED}/${CYNG_LIB}.lib")

		#
		#	debug library
		#
		#	list(APPEND CYNG_LIBRARIES "debug")
		#	list(APPEND CYNG_LIBRARIES "${__CYNG_BUILD_DEBUG}/${CYNG_LIB}.lib")

		#	message(STATUS "** CYNG_LIBRARIES    : ${CYNG_LIBRARIES}")

			#set(${CYNG_LIB} "optimized;${__CYNG_BUILD_OPTIMIZED}/${CYNG_LIB}.lib;debug;${__CYNG_BUILD_DEBUG}/${CYNG_LIB}.lib")
			#message(STATUS "** CYNG_LIB    : ${CYNG_LIB}")

#		endforeach()

#        unset(__CYNG_BUILD_OPTIMIZED CACHE)
#        unset(__CYNG_BUILD_DEBUG CACHE)

#    endif(UNIX)
    
    
    #
    #  remove the cyng prefix - maybe there is a better solution
    #
#     if (UNIX)
# 		get_filename_component(CYNG_INCLUDE_DIR_1 "${CYNG_INCLUDE_DIR_1}/../" REALPATH)
# 	endif()
	set(CYNG_INCLUDE_DIRS "${CYNG_INCLUDE_DIR_SRC};${CYNG_INCLUDE_DIR_BUILD}")
	unset(CYNG_INCLUDE_DIR_SRC CACHE)
	unset(CYNG_INCLUDE_DIR_BUILD CACHE)
    
 #   get_filename_component(CYNG_MODULE_PATH "${CYNG_ROOT_DEV}/src/modules/" REALPATH)
 #   set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CYNG_MODULE_PATH}")

	if (CYNG_INCLUDE_DIRS AND CYNG_LIBRARIES)
		message(STATUS "** CYNG_LIBRARIES    : ${CYNG_LIBRARIES}")
		message(STATUS "** CYNG_INCLUDE_DIRS : ${CYNG_INCLUDE_DIRS}")
		set(CYNG_FOUND ON)
	endif()
    
endif(NOT CYNG_FOUND)


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

if(CYNG_FOUND AND NOT TARGET CYNG::CYNG)

    add_library(CYNG::CYNG INTERFACE IMPORTED)

#	wrong number of arguments
#   set_target_properties(CYNG::CYNG 
#		PROPERTIES
#			INTERFACE_INCLUDE_DIRECTORIES 
#				${CYNG_INCLUDE_DIRS}
#		PROPERTIES
#			INTERFACE_LINK_LIBRARIES 
#				${CYNG_LIBRARIES}
#    )

endif()
