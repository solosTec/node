
option(SMF_TEST_TCP_STRESS "Enable building tcp stress" OFF)

if(SMF_TEST_TCP_STRESS)
add_subdirectory(tcp-stress)
endif()