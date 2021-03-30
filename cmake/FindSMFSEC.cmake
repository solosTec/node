# FindSMFSEC.cmake
#
# Finds the SMFSEC library (formerly known as CRYPTO)
#
# This will define the following variables
#
#   SMFSEC_FOUND          - system has cyng
#   SMFSEC_INCLUDE_DIR    - the crypto include directories
#   SMFSEC_LIBRARIES      - crypto libraries directories
#
# and the following imported targets
#
#     SMFSEC::SMFSEC
#

#
#	try PkgConfig
#
find_package(PkgConfig)
pkg_check_modules(PC_SMFSEC QUIET SMFSEC)

if(SMFSEC_PKG_FOUND)

    set(SMFSEC_VERSION ${PC_SMFSEC_VERSION})
    set(SMFSEC_INCLUDE_DIRS ${SMFSEC_PKG_INCLUDE_DIRS})
    set(SMFSEC_LIBRARIES ${SMFSEC_PKG_LIBRARIES})
       
endif(SMFSEC_PKG_FOUND)

set(SMFSEC_LIBS "smfsec")

if(NOT SMFSEC_FOUND)

	#
	#	cyng header files
	#
    find_path(SMFSEC_INCLUDE_DIR_SRC
        NAMES 
			smfsec/crypto.h
            smfsec/bio.h
        PATH_SUFFIXES
            crypto
        HINTS
            "${PROJECT_SOURCE_DIR}/../crypto/include"
        PATHS
            /usr/include/
            /usr/local/include/
        DOC 
            "SMFSEC headers"
    )
    
	message(STATUS "** SMFSEC_INCLUDE_DIR_SRC: ${SMFSEC_INCLUDE_DIR_SRC}")


	if (WIN32)

	#
	#	search cyng libraries on windows
	#
	foreach(__SMFSEC_LIB ${SMFSEC_LIBS})

		message(STATUS "** hint   : ${__SMFSEC_LIB}")

		find_library("${__SMFSEC_LIB}" ${__SMFSEC_LIB}
			NAMES
				${__SMFSEC_BUILD}
			HINTS
				"${SMFSEC_INCLUDE_DIR_SRC}/../build/Debug"
				"${SMFSEC_INCLUDE_DIR_SRC}/../build/Release"
			PATHS
				/usr/lib/
				/usr/local/lib
			DOC 
				"SMFSEC libraries"
		)
		message(STATUS "** found : ${${__SMFSEC_LIB}}")
		list(APPEND SMFSEC_LIBRARIES ${${__SMFSEC_LIB}})
#		message(STATUS "** SMFSEC_LIBRARIES    : ${SMFSEC_LIBRARIES}")

	endforeach()

	else(UNIX)

	#
	#	search smfsec libraries on linux
	#
	foreach(__SMFSEC_LIB ${SMFSEC_LIBS})

		message(STATUS "** hint : lib${__SMFSEC_LIB}")
		find_library("${__SMFSEC_LIB}" ${__SMFSEC_LIB}
			NAMES
				${__SMFSEC_BUILD}
			HINTS
				"${SMFSEC_INCLUDE_DIR_SRC}/../build"
				"${SMFSEC_INCLUDE_DIR_SRC}/../v5te"
				"${SMFSEC_INCLUDE_DIR_SRC}/../build/x64"
				"${SMFSEC_INCLUDE_DIR_SRC}/../build/v5te"
			PATHS
				/usr/lib/
				/usr/local/lib
			DOC 
				"SMFSEC libraries"
		)
		message(STATUS "** found : ${${__SMFSEC_LIB}}")
		list(APPEND SMFSEC_LIBRARIES ${${__SMFSEC_LIB}})
#		message(STATUS "** SMFSEC_LIBRARIES    : ${SMFSEC_LIBRARIES}")

	endforeach()

	endif()

    
	set(SMFSEC_INCLUDE_DIRS "${SMFSEC_INCLUDE_DIR_SRC}")
	unset(SMFSEC_INCLUDE_DIR_SRC CACHE)
    

	if (SMFSEC_INCLUDE_DIRS AND SMFSEC_LIBRARIES)
		message(STATUS "** SMFSEC_LIBRARIES        : ${SMFSEC_LIBRARIES}")
		message(STATUS "** SMFSEC_INCLUDE_DIRS     : ${SMFSEC_INCLUDE_DIRS}")
		set(SMFSEC_FOUND ON)
	endif()
    
endif(NOT SMFSEC_FOUND)


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

if(SMFSEC_FOUND AND NOT TARGET SMFSEC::SMFSEC)

    add_library(SMFSEC::SMFSEC INTERFACE IMPORTED)

#	define a target
   	set_target_properties(SMFSEC::SMFSEC 
		PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES 
				"${SMFSEC_INCLUDE_DIRS}"
			INTERFACE_LINK_LIBRARIES 
				"${SMFSEC_LIBRARIES}"
    )

endif()