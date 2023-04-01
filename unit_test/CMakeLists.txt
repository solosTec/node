# Dependencies
find_package(Boost 1.75 REQUIRED COMPONENTS unit_test_framework)

message(STATUS "** prepare unit tests **")

set(test_assets
    assets/smf--SML-202012T071425-power@solostec8bbb6264-bc4a4b92.sml
    assets/iec.log    
)

source_group("test-assets" FILES ${test_assets}) 

add_executable(unit_test 
    main.cpp
    src/test_config.cpp
    src/test_sml.cpp
    src/test_iec.cpp
    src/test_ipt.cpp
    src/test_mbus.cpp
    src/test_obis.cpp
    src/test_report.cpp
    src/test_dns.cpp
    ${test_assets}
)

target_compile_features(unit_test PRIVATE cxx_std_20)

target_compile_definitions(unit_test
    PRIVATE 
        BOOST_TEST_DYN_LINK 
        BOOST_ASIO_HAS_MOVE
        $<$<CONFIG:Debug>:_DEBUG_UNIT_TEST>
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
        smf::config
        smf::sml
        smf::iec
        smf::ipt
        smf::mbus
        smf::obis
        smf::dns
        smfsec::smfsec
        cyng::cyng
        ${SMFSEC_LIBRARIES}
        OpenSSL::SSL
        "$<$<PLATFORM_ID:Windows>:odbc32;pugixml::pugixml>"
        "$<$<PLATFORM_ID:Linux>:Boost::thread;Boost::system;Boost::program_options;Boost::random;Boost::locale;${CMAKE_DL_LIBS};atomic;pthread>"        
#        Boost::thread # not found
        ${Boost_THREAD_LIBRARY}
        Boost::unit_test_framework
        $<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)

# include(CTest)
enable_testing()

add_test(DemoTest unit_test)

