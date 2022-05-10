
option(SMF_TEST_TCP_STRESS "Enable building tcp stress" ON)

if(SMF_TEST_TCP_STRESS)
add_subdirectory(tcp-stress)
endif()