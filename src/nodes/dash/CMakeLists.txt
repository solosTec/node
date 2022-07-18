
include (node.cmake)

add_executable(dash ${dash_node})

target_compile_features(dash PRIVATE cxx_std_20)

target_compile_definitions(dash
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_DASH>
)

#
# -Wno-psabi for gcc > 7.1
#
target_compile_options(dash 
    PRIVATE 
        $<$<AND:$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},7.1>,$<CXX_COMPILER_ID:GNU>>:-Wno-psabi>
)

target_include_directories(dash
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${OPENSSL_INCLUDE_DIR}
    PRIVATE
        ${SMFSEC_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
)

target_link_directories(dash
    PRIVATE
        ${Boost_LIBRARY_DIRS}
)

target_link_libraries(dash 
    PRIVATE
        smf::config
        smf::cluster_bus
        smf::http
        smf::mbus
        smf::sml
        smf::obis
        cyng::cyng 
        "$<$<PLATFORM_ID:Windows>:pugixml>"
        ${SMFSEC_LIBRARIES}
        OpenSSL::SSL
        "$<$<PLATFORM_ID:Linux>:Boost::thread;Boost::system;Boost::program_options;Boost::random;Boost::locale;${CMAKE_DL_LIBS};atomic;pthread>"
        $<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)


if(UNIX)
    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/linux.cgf.in"
		    "${PROJECT_BINARY_DIR}/config/dash_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/unit.in"
		    "${PROJECT_BINARY_DIR}/config/dash.service")

else()
    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/windows.cgf.in"
		    "${PROJECT_BINARY_DIR}/config/dash_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/create_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/dash_create_service.cmd")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/delete_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/dash_delete_service.cmd")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/restart_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/dash_restart_service.cmd")

#    configure_file (
#		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/rc.in"
#		    "${PROJECT_BINARY_DIR}/dash.rc")

endif()


