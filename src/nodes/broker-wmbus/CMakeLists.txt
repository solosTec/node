
include (node.cmake)

add_executable(broker-wmbus ${broker-wmbus_node})

target_compile_features(broker-wmbus PRIVATE cxx_std_20)

target_compile_definitions(broker-wmbus
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_BROKER_WMBUS>
)

#
# -Wno-psabi for gcc > 7.1
#
target_compile_options(broker-wmbus 
    PRIVATE 
        $<$<AND:$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},7.1>,$<CXX_COMPILER_ID:GNU>>:-Wno-psabi>
)

target_include_directories(broker-wmbus
    PRIVATE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Boost_INCLUDE_DIRS}
)

target_link_directories(broker-wmbus
    PRIVATE
        ${Boost_LIBRARY_DIRS}
)

target_link_libraries(broker-wmbus 
    PRIVATE
        smf_config
        smf_cluster_bus
        smf_mbus
        smf_sml
        smf_ipt_bus
        smf_ipt
        ${CYNG_LIBRARIES} 
#        ${SMFSEC_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        "$<$<PLATFORM_ID:Linux>:Boost::thread;Boost::system;Boost::program_options;Boost::random;Boost::locale;smfsec::smfsec;${CMAKE_DL_LIBS};atomic;pthread>"        
        $<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

if(UNIX)
    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/linux.cgf.in"
		    "${PROJECT_BINARY_DIR}/config/broker-wmbus_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/unit.in"
		    "${PROJECT_BINARY_DIR}/config/broker-wmbus.service")

else()
    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/windows.cgf.in"
		    "${PROJECT_BINARY_DIR}/config/broker-wmbus_v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/create_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/broker-wmbus_create_service.cmd")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/delete_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/broker-wmbus_delete_service.cmd")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/restart_service.cmd.in"
		    "${PROJECT_BINARY_DIR}/config/broker-wmbus_restart_service.cmd")

#    configure_file (
#		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/rc.in"
#		    "${PROJECT_BINARY_DIR}/broker-wmbus.rc")

endif()



