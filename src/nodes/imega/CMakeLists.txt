
include (node.cmake)

add_executable(imega ${imega_node})

target_compile_features(imega PRIVATE cxx_std_20)

target_compile_definitions(imega
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_IMEGA>
)

#
# -Wno-psabi for gcc > 7.1
#
target_compile_options(imega 
    PRIVATE 
        $<$<AND:$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},7.1>,$<CXX_COMPILER_ID:GNU>>:-Wno-psabi>
)

target_include_directories(imega
    PRIVATE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Boost_INCLUDE_DIRS}
)

target_link_directories(imega
    PRIVATE
        ${Boost_LIBRARY_DIRS}
)

target_link_libraries(imega 
    PRIVATE
        smf::config
        smf::cluster
        smf::cluster_bus
        cyng::obj
        cyng::parse
        cyng::io
        cyng::log
        cyng::sys
        cyng::task
        cyng::vm
		"$<$<PLATFORM_ID:Windows>:odbc32;pugixml::pugixml>"
        "$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${Boost_LOCALE_LIBRARY};${CMAKE_DL_LIBS};atomic;pthread>"
        $<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

if(UNIX)
    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/linux.cgf.in"
		    "${PROJECT_BINARY_DIR}/config/imega_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/unit.in"
		    "${PROJECT_BINARY_DIR}/config/imega.service")

else()
    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/windows.cgf.in"
		    "${PROJECT_BINARY_DIR}/config/imega_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/create_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/imega_create_service.cmd")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/delete_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/imega_delete_service.cmd")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/restart_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/imega_restart_service.cmd")

#    configure_file (
#		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/rc.in"
#		    "${PROJECT_BINARY_DIR}/imega.rc")

endif()




