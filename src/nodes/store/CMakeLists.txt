
include (node.cmake)

add_executable(store ${store_node})

target_compile_features(store PRIVATE cxx_std_20)

target_compile_definitions(store
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_STORE>
)

#
# -Wno-psabi for gcc > 7.1
#
target_compile_options(store 
    PRIVATE 
        $<$<AND:$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},7.1>,$<CXX_COMPILER_ID:GNU>>:-Wno-psabi>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/ZI>
)

target_include_directories(store
    PRIVATE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Boost_INCLUDE_DIRS}
)

target_link_directories(store
    PRIVATE
        ${Boost_LIBRARY_DIRS}
)

target_link_libraries(store 
    PRIVATE
        smf::report
        smf::config
        smf::ipt
        smf::ipt_bus
        smf::sml
        smf::iec
        smf::mbus
        smf::obis
        cyng::cyng 
        "$<$<PLATFORM_ID:Windows>:pugixml>"
        "$<$<PLATFORM_ID:Linux>:${OPENSSL_LIBRARIES}>"
        "$<$<PLATFORM_ID:Linux>:Boost::thread;Boost::system;Boost::program_options;Boost::random;Boost::locale;${CMAKE_DL_LIBS};atomic;pthread>"        
        $<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

if(UNIX)
    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/linux.cgf.in"
		    "${PROJECT_BINARY_DIR}/config/store_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/unit.in"
		    "${PROJECT_BINARY_DIR}/config/store.service")

else()
    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/windows.cgf.in"
		    "${PROJECT_BINARY_DIR}/config/store_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/create_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/store_create_service.cmd")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/delete_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/store_delete_service.cmd")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/restart_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/store_restart_service.cmd")

#    configure_file (
#		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/rc.in"
#		    "${PROJECT_BINARY_DIR}/store.rc")

endif()

