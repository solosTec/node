
option(BUILD_TCP_STRESS "Enable building tcp stress" ON)

if(BUILD_TCP_STRESS)
add_subdirectory(tcp-stress)
endif()