# FindCRYPTO.cmake
#
# Finds the CRYPTO library
#
# This will define the following variables
#
#   CRYPTO_FOUND          - system has cyng
#   CRYPTO_INCLUDE_DIR    - the crypto include directories
#   CRYPTO_LIBRARIES      - crypto libraries directories
#
# and the following imported targets
#
#     CRYPTO::CRYPTO
#

#
#	try PkgConfig
#
find_package(PkgConfig)
pkg_check_modules(PC_CRYPTO QUIET CRYPTO)

if(CRYPTO_PKG_FOUND)

    set(CRYPTO_VERSION ${PC_CRYPTO_VERSION})
    set(CRYPTO_INCLUDE_DIRS ${CRYPTO_PKG_INCLUDE_DIRS})
    set(CRYPTO_LIBRARIES ${CRYPTO_PKG_LIBRARIES})
       
endif(CRYPTO_PKG_FOUND)

set(CRYPTO_LIBS "crypto")

if(NOT CRYPTO_FOUND)

	#
	#	cyng header files
	#
    find_path(CRYPTO_INCLUDE_DIR_SRC
        NAMES 
            crypto/crypto.h
            crypto/bio.h
        PATH_SUFFIXES
            crypto
        HINTS
            "${PROJECT_SOURCE_DIR}/../crypto/include"
        PATHS
            /usr/include/
            /usr/local/include/
        DOC 
            "CRYPTO headers"
    )
    
	message(STATUS "** CRYPTO_INCLUDE_DIR_SRC: ${CRYPTO_INCLUDE_DIR_SRC}")


	if (WIN32)

	#
	#	search cyng libraries on windows
	#
	foreach(__CRYPTO_LIB ${CRYPTO_LIBS})

		message(STATUS "** hint   : ${__CRYPTO_LIB}")

		find_library("${__CRYPTO_LIB}" ${__CRYPTO_LIB}
			NAMES
				${__CRYPTO_BUILD}
			HINTS
				"${CRYPTO_INCLUDE_DIR_SRC}/../build/Debug"
				"${CRYPTO_INCLUDE_DIR_SRC}/../build/Release"
			PATHS
				/usr/lib/
				/usr/local/lib
			DOC 
				"CRYPTO libraries"
		)
		message(STATUS "** found : ${${__CRYPTO_LIB}}")
		list(APPEND CRYPTO_LIBRARIES ${${__CRYPTO_LIB}})
#		message(STATUS "** CRYPTO_LIBRARIES    : ${CRYPTO_LIBRARIES}")

	endforeach()

	else(UNIX)

	#
	#	search cyng libraries on linux
	#
	foreach(__CRYPTO_LIB ${CRYPTO_LIBS})

		message(STATUS "** hint : lib${__CRYPTO_LIB}")
		find_library("${__CRYPTO_LIB}" ${__CRYPTO_LIB}
			NAMES
				${__CRYPTO_BUILD}
			HINTS
				"${CRYPTO_INCLUDE_DIR_SRC}/../build"
				"${CRYPTO_INCLUDE_DIR_SRC}/../v5te"
				"${CRYPTO_INCLUDE_DIR_SRC}/../build/x64"
				"${CRYPTO_INCLUDE_DIR_SRC}/../build/v5te"
			PATHS
				/usr/lib/
				/usr/local/lib
			DOC 
				"CRYPTO libraries"
		)
		message(STATUS "** found : ${${__CRYPTO_LIB}}")
		list(APPEND CRYPTO_LIBRARIES ${${__CRYPTO_LIB}})
#		message(STATUS "** CRYPTO_LIBRARIES    : ${CRYPTO_LIBRARIES}")

	endforeach()

	endif()

    
	set(CRYPTO_INCLUDE_DIRS "${CRYPTO_INCLUDE_DIR_SRC}")
	unset(CRYPTO_INCLUDE_DIR_SRC CACHE)
    

	if (CRYPTO_INCLUDE_DIRS AND CRYPTO_LIBRARIES)
		message(STATUS "** CRYPTO_LIBRARIES        : ${CRYPTO_LIBRARIES}")
		message(STATUS "** CRYPTO_INCLUDE_DIRS     : ${CRYPTO_INCLUDE_DIRS}")
		set(CRYPTO_FOUND ON)
	endif()
    
endif(NOT CRYPTO_FOUND)


mark_as_advanced(CRYPTO_FOUND CRYPTO_INCLUDE_DIRS CRYPTO_LIBRARIES)

if(UNIX)
	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(CRYPTO
		REQUIRED_VARS 
			CRYPTO_INCLUDE_DIRS
			CRYPTO_LIBRARIES
		VERSION_VAR 
			CRYPTO_VERSION
		FAIL_MESSAGE
			"Cannot provide CRYPTO library"
	)
endif(UNIX)

if(CRYPTO_FOUND AND NOT TARGET CRYPTO::CRYPTO)

    add_library(CRYPTO::CRYPTO INTERFACE IMPORTED)

#	define a target
   	set_target_properties(CRYPTO::CRYPTO 
		PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES 
				"${CRYPTO_INCLUDE_DIRS}"
			INTERFACE_LINK_LIBRARIES 
				"${CRYPTO_LIBRARIES}"
    )

endif()
