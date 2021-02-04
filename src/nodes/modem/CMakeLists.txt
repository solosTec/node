
include (node.cmake)

add_executable(modem ${modem_node})

target_compile_features(modem PRIVATE cxx_std_20)

target_compile_definitions(modem
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_FRONTEND>
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
        ${CYNG_LIBRARIES} 
        smf_config
        $<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)



