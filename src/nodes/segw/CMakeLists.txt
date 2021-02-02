
include (node.cmake)

add_executable(segw ${segw_node})

target_compile_features(segw PRIVATE cxx_std_20)

target_compile_definitions(segw
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_FRONTEND>
)

#
# -Wno-psabi for gcc > 7.1
#
target_compile_options(segw 
    PRIVATE 
        $<$<AND:$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},7.1>,$<CXX_COMPILER_ID:GNU>>:-Wno-psabi>
)

target_include_directories(segw
    PRIVATE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_directories(segw
    PRIVATE
        ${Boost_LIBRARY_DIRS}
)

target_link_libraries(segw 
    PRIVATE
        Boost::program_options
        $<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)



