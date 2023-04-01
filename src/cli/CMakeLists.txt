
include (node.cmake)
 
#
#
#
#set(BUILD_SHARED_LIBS OFF);

set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
    GIT_TAG "v4.0.0"
)
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

message(STATUS "** ftxui sources    : ${ftxui_SOURCE_DIR}") 
message(STATUS "** ftxui binaries   : ${ftxui_BINARY_DIR}") 

project(smfgui
  LANGUAGES CXX
  VERSION 1.0.0
)
 
include (node.cmake)
add_executable(smfcli ${smf_cli})


target_compile_features(smfcli PRIVATE 
    cxx_alignas
    cxx_std_20
    cxx_auto_type
    cxx_constexpr
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
)


target_include_directories(smfcli 
    PRIVATE 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Boost_INCLUDE_DIRS}
)

target_link_directories(smfcli
    PRIVATE
        ${Boost_LIBRARY_DIRS}
)

target_link_libraries(smfcli
    PRIVATE
        smf::config
        smf::mbus
        smf::sml
        smf::obis
        cyng::cyng 
        "$<$<PLATFORM_ID:Windows>:odbc32;pugixml::pugixml>"
        "$<$<PLATFORM_ID:Linux>:Boost::thread;Boost::system;Boost::program_options;Boost::random;Boost::locale;${CMAKE_DL_LIBS};atomic;pthread>"        
        ftxui::screen
        ftxui::dom
        ftxui::component # Not needed for this example.
)
