
include (node.cmake)

add_executable(modem ${modem_node})

target_compile_features(modem PRIVATE cxx_std_20)

target_compile_definitions(modem
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_MODEM>
)

#
# -Wno-psabi for gcc > 7.1
#
target_compile_options(modem 
    PRIVATE 
        $<$<AND:$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},7.1>,$<CXX_COMPILER_ID:GNU>>:-Wno-psabi>
)

target_include_directories(modem
    PRIVATE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Boost_INCLUDE_DIRS}
)

target_link_directories(modem
    PRIVATE
        ${Boost_LIBRARY_DIRS}
)

target_link_libraries(modem 
    PRIVATE
        cyng::cyng 
        "$<$<PLATFORM_ID:Windows>:pugixml>"
        smf::config
        smf::cluster
        smf::cluster_bus
        smf::modem
        "$<$<PLATFORM_ID:Linux>:Boost::thread;Boost::system;Boost::program_options;Boost::random;Boost::locale;${CMAKE_DL_LIBS};atomic;pthread>"        
        $<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

if(UNIX)
    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/linux.cgf.in"
		    "${PROJECT_BINARY_DIR}/config/modem_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/unit.in"
		    "${PROJECT_BINARY_DIR}/config/modem.service")

else()
    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/windows.cgf.in"
		    "${PROJECT_BINARY_DIR}/config/modem_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/create_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/modem_create_service.cmd")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/delete_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/modem_delete_service.cmd")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/restart_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/modem_restart_service.cmd")

#    configure_file (
#		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/rc.in"
#		    "${PROJECT_BINARY_DIR}/modem.rc")

endif()

