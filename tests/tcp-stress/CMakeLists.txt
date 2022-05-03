cmake_minimum_required (VERSION 3.14)
project(tcpstress)

add_executable(tcp_server server/main.cpp)

#
#   The dependency from Boost provides also the include path
#   to the Boost library
#
target_link_libraries(tcp_server
    PRIVATE 
        Boost::boost 
)

#message(STATUS "** Boost Version numeric        : ${Boost_VERSION_NUMERIC}")
#message(STATUS "** Boost Include                : ${Boost_INCLUDE_DIRS}")
#message(STATUS "** Boost Path                   : ${Boost_LIBRARY_DIRS}")
#message(STATUS "** Boost_PROGRAM_OPTIONS_LIBRARY: ${Boost_PROGRAM_OPTIONS_LIBRARY}")

add_executable(tcp_client client/main.cpp)

#
#   For an unknown reason the Boost.Program_options will not be found on windows
#
target_link_libraries(tcp_client 
    PRIVATE 
        Boost::boost 
		"$<$<PLATFORM_ID:Windows>:C:/local/boost_1_79_0/lib64-msvc-14.3/libboost_program_options-vc143-mt-gd-x64-1_79.lib>"

)
