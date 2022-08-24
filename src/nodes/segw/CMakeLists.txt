project(segw)
                                               
message(STATUS "                                           .---. ")
message(STATUS "   .--.--.               ,----._,.        /. ./| ")
message(STATUS "  /  /    '     ,---.   /   /  ' /     .-'-. ' | ")
message(STATUS " |  :  /`./    /     \\ |   :     |    /___/ \\: | ")
message(STATUS " |  :  ;_     /    /  ||   | .\\  . .-'.. '   ' . ")
message(STATUS "  \\  \\    `. .    ' / |.   ; ';  |/___/ \\:     ' ")
message(STATUS "   `----.   \\'   ;   /|'   .   . |.   \\  ' .\\    ")
message(STATUS "  /  /`--'  /'   |  / | `---`-'| | \\   \\   ' \\ | ")
message(STATUS " '--'.     / |   :    | .'__/\\_: |  \\   \\  |--\"  ")
message(STATUS "   `--'---'   \\   \\  /  |   :    :   \\   \\ |     ")
message(STATUS "               `----'    \\   \\  /     '---\"      ")
message(STATUS "                          `--`-'                 ")

message(STATUS "CMAKE_PROJECT_NAME       : ${CMAKE_PROJECT_NAME}")
message(STATUS "PROJECT_NAME             : ${PROJECT_NAME}")
message(STATUS "CMAKE_BINARY_DIR         : ${CMAKE_BINARY_DIR}")
message(STATUS "PROJECT_BINARY_DIR       : ${PROJECT_BINARY_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR : ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "version                  : v${${CMAKE_PROJECT_NAME}_VERSION_MAJOR}.${${CMAKE_PROJECT_NAME}_VERSION_MINOR}")

include (node.cmake)

add_executable(segw ${segw_node})

target_compile_features(segw PRIVATE cxx_std_20)

target_compile_definitions(segw
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_SEGW>
        $<IF:$<BOOL:${CMAKE_CROSSCOMPILING}>,__CROSS_PLATFORM,__NATIVE_PLATFORM>
)

#
# -Wno-psabi for gcc > 7.1
# EDITANDCONTINUE
#
target_compile_options(segw 
    PRIVATE 
        $<$<AND:$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},7.1>,$<CXX_COMPILER_ID:GNU>>:-Wno-psabi>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/ZI>
)

target_include_directories(segw
    PRIVATE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Boost_INCLUDE_DIRS}
)

target_link_directories(segw
    PRIVATE
        ${Boost_LIBRARY_DIRS}
)

target_link_libraries(segw 
    PRIVATE
        cyng::cyng 
        "$<$<PLATFORM_ID:Windows>:pugixml::pugixml>"
        smfsec::smfsec
        smf::config
        smf::ipt
        smf::ipt_bus
        smf::serial_bus
        smf::hci
        smf::mbus
        smf::obis
        smf::sml
        OpenSSL::SSL
		"$<$<PLATFORM_ID:Windows>:odbc32>"
        "$<$<PLATFORM_ID:Linux>:Boost::thread;Boost::system;Boost::program_options;Boost::random;Boost::locale;${CMAKE_DL_LIBS};atomic;pthread>"        
        $<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

if(UNIX)

    if (${${PROJECT_NAME}_CROSS_COMPILE})
	    configure_file (
			    "${CMAKE_CURRENT_SOURCE_DIR}/templates/${PROJECT_NAME}.oecp.cgf.in"
			    "${CMAKE_BINARY_DIR}/config/${PROJECT_NAME}_v${${CMAKE_PROJECT_NAME}_VERSION_MAJOR}.${${CMAKE_PROJECT_NAME}_VERSION_MINOR}.cfg")
			
	    configure_file (
			    "${CMAKE_CURRENT_SOURCE_DIR}/templates/oecp.service.in"
			    "${CMAKE_BINARY_DIR}/config/${PROJECT_NAME}.service")
    else()

        configure_file (
		        "${CMAKE_CURRENT_SOURCE_DIR}/templates/linux.cgf.in"
		        "${CMAKE_BINARY_DIR}/config/${PROJECT_NAME}_v${${CMAKE_PROJECT_NAME}_VERSION_MAJOR}.${${CMAKE_PROJECT_NAME}_VERSION_MINOR}.cfg")

        configure_file (
		        "${CMAKE_CURRENT_SOURCE_DIR}/templates/unit.in"
		        "${CMAKE_BINARY_DIR}/config/${PROJECT_NAME}.service")
    endif()
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

