
include (node.cmake)
 
#
#
#
#set(BUILD_SHARED_LIBS OFF);

set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
    GIT_TAG "v3.0.0"
)
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

#FetchContent_MakeAvailable(ftxui)

project(smfgui
  LANGUAGES CXX
  VERSION 1.0.0
)
 
#add_executable(smfgui src/gui.cpp)
#target_include_directories(ftxui-starter PRIVATE src)
 
#target_link_libraries(smfgui
#  PRIVATE ftxui::screen
#  PRIVATE ftxui::dom
#  PRIVATE ftxui::component # Not needed for this example.
#)
#target_include_directories(smfgui PRIVATE src)

add_executable(smfcli src/main.cpp)
target_include_directories(smfcli PRIVATE src)
