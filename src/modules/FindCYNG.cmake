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

find_package(PkgConfig)
pkg_check_modules(PC_CYNG QUIET CYNG)

if(CYNG_PKG_FOUND)

    set(CYNG_VERSION ${PC_CYNG_VERSION})
    set(CYNG_INCLUDE_DIRS ${CYNG_PKG_INCLUDE_DIRS})
    set(CYNG_LIBRARIES ${CYNG_PKG_LIBRARIES})

else(CYNG_PKG)

    set(__CYNG_DEVELOP_ROOT "${PROJECT_SOURCE_DIR}/../cyng" CACHE PATH "__CYNG_DEVELOP_ROOT")
    get_filename_component(CYNG_DEVELOP_ROOT ${__CYNG_DEVELOP_ROOT} REALPATH)
    message(STATUS "** search for developer path: ${CYNG_DEVELOP_ROOT}")

    find_path(CYNG_INCLUDE_DIR_1
        NAMES 
            cyng/cyng.h
            cyng/object.h
        PATH_SUFFIXES
            cyng
        HINTS
            "${CYNG_DEVELOP_ROOT}/src/main/include"
        PATHS
            /usr/include/
            /usr/local/include/
        DOC 
            "CYNG headers"
    )
    
#     message(STATUS "** found 1: ${CYNG_INCLUDE_DIR}")

    find_path(CYNG_INCLUDE_DIR_2
        NAMES 
            CYNG_project_info.h
         HINTS
            "${CYNG_DEVELOP_ROOT}/build"
            /usr/include/
            /usr/local/include/
        DOC 
            "CYNG headers"
    )

#     message(STATUS "** found 2: ${CYNG_INCLUDE_DIR_2}")

    if (UNIX)
        set(__CYNG_BUILD "${CYNG_DEVELOP_ROOT}/build" CACHE PATH "__CYNG_BUILD")
    else()
        #
        #	$(ConfigurationName) is a variable used by the MS build system
        #
        set(__CYNG_BUILD "${CYNG_DEVELOP_ROOT}/build/$(ConfigurationName)" CACHE PATH "__CYNG_BUILD")
    endif(UNIX)
    
    get_filename_component(CYNG_BUILD ${__CYNG_BUILD} REALPATH)
#     message(STATUS "** search for build path: ${CYNG_BUILD}")

    # libcyng_async.so   libcyng_csv.so        libcyng_domain.so  libcyng_log.so   libcyng_parser.so  libcyng_store.so  libcyng_vm.so
    # libcyng_core.so    libcyng_db.so         libcyng_io.so      libcyng_mail.so  libcyng_rnd.so     libcyng_sys.so    libcyng_xml.so
    # libcyng_json.so    libcyng_meta.so       libcyng_sql.so     libcyng_table.so

    set(CYNG_LIBRARIES "")
    set(CYNG_LIB_LIST "cyng_async;cyng_csv;cyng_domain;cyng_log;cyng_parser;cyng_store;cyng_vm;cyng_core;cyng_db;cyng_io;cyng_crypto;cyng_mail;cyng_rnd;cyng_sys;cyng_xml;cyng_json;cyng_meta;cyng_sql;cyng_table;cyng_sqlite3")
    
    foreach(CYNG_LIB ${CYNG_LIB_LIST})
        find_library("__${CYNG_LIB}" ${CYNG_LIB}
            HINTS
                ${CYNG_BUILD}
            PATHS
                /usr/lib/
                /usr/local/lib
            DOC 
                "CYNG libraries"
        )
        
        list(APPEND CYNG_LIBRARIES ${__${CYNG_LIB}})
#         message(STATUS "** __CYNG_LIB    : ${__${CYNG_LIB}}")
    endforeach()

    #
    #  remove the cyng prefix - maybe there is a better solution
    #
    get_filename_component(CYNG_INCLUDE_DIR_1 "${CYNG_INCLUDE_DIR_1}/../" REALPATH)
    set(CYNG_INCLUDE_DIRS "${CYNG_INCLUDE_DIR_1};${CYNG_INCLUDE_DIR_2}")
    
    get_filename_component(CYNG_MODULE_PATH "${CYNG_DEVELOP_ROOT}/src/modules/" REALPATH)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CYNG_MODULE_PATH}")
#     message(STATUS "** CMake modules     : ${CMAKE_MODULE_PATH}")
    
endif(CYNG_PKG_FOUND)


mark_as_advanced(CYNG_FOUND CYNG_INCLUDE_DIRS CYNG_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CYNG
    REQUIRED_VARS 
        CYNG_INCLUDE_DIRS
        CYNG_LIBRARIES
    VERSION_VAR 
        CYNG_VERSION
)

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
