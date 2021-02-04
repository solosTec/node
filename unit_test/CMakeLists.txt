# Dependencies
find_package(Boost 1.75 REQUIRED COMPONENTS unit_test_framework)

add_executable(unit_test 
    main.cpp
    src/test_core.cpp
    src/test_task.cpp
    src/test_io.cpp
    src/test_vm.cpp
    src/test_store.cpp
    src/test_log.cpp
    src/demo_task.cpp
    src/demo_task_ref.cpp)

target_compile_features(unit_test PRIVATE cxx_std_20)

target_compile_definitions(unit_test
    PRIVATE 
        BOOST_TEST_DYN_LINK 
        BOOST_ASIO_HAS_MOVE 
        $<$<CONFIG:Debug>:_DEBUG_OBJECT>
)

#
# -Wno-psabi for gcc > 7.1
#
target_compile_options(unit_test 
    PRIVATE 
        $<$<AND:$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},7.1>,$<CXX_COMPILER_ID:GNU>>:-Wno-psabi>
)

target_include_directories(unit_test
    PRIVATE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
message(STATUS "** Boost_UNIT_TEST_FRAMEWORK_LIBRARY      : ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY_DEBUG}")

target_link_directories(unit_test
    PRIVATE
        ${Boost_LIBRARY_DIRS}
)

target_link_libraries(unit_test 
    PRIVATE
        cyng_obj
        cyng_task
        cyng_io
        cyng_vm
        cyng_log
        cyng_store
        "$<$<PLATFORM_ID:Linux>:pthread>"
        "$<$<PLATFORM_ID:Windows>:odbc32>"
        "$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};atomic;pthread>"
        Boost::date_time 
        Boost::unit_test_framework
        $<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

# include(CTest)
enable_testing()

add_test(DemoTest unit_test)

