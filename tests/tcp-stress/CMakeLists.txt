cmake_minimum_required (VERSION 3.14)
project(tcpstress)

add_executable("tcp-server" server/main.cpp)

#
#   The dependency from Boost provides also the include path
#   to the Boost library
#
target_link_libraries("tcp-server" 
    PRIVATE 
        Boost::boost 
)

add_executable("tcp-client" client/main.cpp)

target_link_libraries("tcp-client" 
    PRIVATE 
        cyng::cyng 
        Boost::boost 
)
