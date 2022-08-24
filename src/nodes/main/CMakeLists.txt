project(main)                                                
                                                 
message(STATUS "            ____                                   ")
message(STATUS "          ,'  , `.             ,--,                ")
message(STATUS "       ,-+-,.' _ |           ,--.'|         ,---,  ")
message(STATUS "    ,-+-. ;   , ||           |  |,      ,-+-. /  | ")
message(STATUS "   ,--.'|'   |  || ,--.--.   `--'_     ,--.'|'   | ")
message(STATUS "  |   |  ,', |  |,/       \\  ,' ,'|   |   |  ,\"' | ")
message(STATUS "  |   | /  | |--'.--.  .-. | '  | |   |   | /  | | ")
message(STATUS "  |   : |  | ,    \\__\\/: . . |  | :   |   | |  | | ")
message(STATUS "  |   : |  |/     ,\" .--.; | '  : |__ |   | |  |/  ")
message(STATUS "  |   | |`-'     /  /  ,.  | |  | '.'||   | |--'   ")
message(STATUS "  |   ;/        ;  :   .'   \\;  :    ;|   |/       ")
message(STATUS "  '---'         |  ,     .-./|  ,   / '---'        ")
message(STATUS "                 `--`---'     ---`-'               ")
                                                 
message(STATUS "CMAKE_PROJECT_NAME       : ${CMAKE_PROJECT_NAME}")
message(STATUS "PROJECT_NAME             : ${PROJECT_NAME}")
message(STATUS "CMAKE_BINARY_DIR         : ${CMAKE_BINARY_DIR}")
message(STATUS "PROJECT_BINARY_DIR       : ${PROJECT_BINARY_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR : ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "version                  : v${${CMAKE_PROJECT_NAME}_VERSION_MAJOR}.${${CMAKE_PROJECT_NAME}_VERSION_MINOR}")                                                

include (node.cmake)

add_executable(main ${main_node})

target_compile_features(main PRIVATE cxx_std_20)

target_compile_definitions(main
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_MAIN>
)

#
# -Wno-psabi for gcc > 7.1
#
target_compile_options(main 
    PRIVATE 
        $<$<AND:$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},7.1>,$<CXX_COMPILER_ID:GNU>>:-Wno-psabi>
)

target_include_directories(main
    PRIVATE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Boost_INCLUDE_DIRS}
)

target_link_directories(main
    PRIVATE
        ${Boost_LIBRARY_DIRS}
)

target_link_libraries(main 
    PRIVATE
        smf::config
        cyng::cyng 
        "$<$<PLATFORM_ID:Windows>:pugixml::pugixml>"
        "$<$<PLATFORM_ID:Linux>:Boost::thread;Boost::system;Boost::program_options;Boost::random;Boost::locale;${CMAKE_DL_LIBS};atomic;pthread>"        
        $<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

if(UNIX)
    set(SMF_UNIT_FILE_MAIN "${CMAKE_BINARY_DIR}/config/${PROJECT_NAME.service")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/linux.cgf.in"
		    "${CMAKE_BINARY_DIR}/config/${PROJECT_NAME}_v${${CMAKE_PROJECT_NAME}_VERSION_MAJOR}.${${CMAKE_PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/unit.in"
		    ${SMF_UNIT_FILE_MAIN})

# https://github.com/andrew-hardin/cpack-systemd-demo/blob/master/CMakeLists.txt            
    install(FILES ${SMF_UNIT_FILE_MAIN} DESTINATION share)

else()
    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/windows.cgf.in"
		    "${CMAKE_BINARY_DIR}/config/${PROJECT_NAME}_v${${CMAKE_PROJECT_NAME}_VERSION_MAJOR}.${${CMAKE_PROJECT_NAME}_VERSION_MINOR}.cfg")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/create_service.cmd.in"
		    "${CMAKE_BINARY_DIR}/config/${PROJECT_NAME}_create_service.cmd")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/delete_service.cmd.in"
		    "${CMAKE_BINARY_DIR}/config/${PROJECT_NAME}_delete_service.cmd")

    configure_file (
		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/restart_service.cmd.in"
		    "${CMAKE_BINARY_DIR}/config/${PROJECT_NAME}_restart_service.cmd")

#    configure_file (
#		    "${CMAKE_CURRENT_SOURCE_DIR}/templates/rc.in"
#		    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.rc")

endif()

