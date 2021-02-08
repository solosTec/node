
include (tool.cmake)

add_executable(obis ${obis_tool})

target_compile_features(obis PRIVATE cxx_std_20)

target_compile_definitions(obis
    PRIVATE 
        $<$<CONFIG:Debug>:_DEBUG_OBIS>
        "__OUTPUT_PATH=\"${CMAKE_SOURCE_DIR}/include/smf/obis/defs.ipp\""
)

#
# -Wno-psabi for gcc > 7.1
#
target_compile_options(obis 
    PRIVATE 
        $<$<AND:$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},7.1>,$<CXX_COMPILER_ID:GNU>>:-Wno-psabi>
)

target_include_directories(obis
    PRIVATE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Boost_INCLUDE_DIRS}
)

target_link_directories(obis
    PRIVATE
        ${Boost_LIBRARY_DIRS}
)

target_link_libraries(obis 
    PRIVATE
#        Boost::program_options
        ${CYNG_LIBRARIES} 
        smf_obis
        "$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};atomic;pthread>"
        $<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)



