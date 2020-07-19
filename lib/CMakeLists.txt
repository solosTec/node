#
#	cluster library (client side)
#
add_subdirectory(cluster)
add_subdirectory(hci)
add_subdirectory(http)
add_subdirectory(https)
add_subdirectory(iec)
add_subdirectory(imega)
add_subdirectory(info)
add_subdirectory(ipt)
add_subdirectory(lora)
add_subdirectory(mbus)
add_subdirectory(modem)
add_subdirectory(serial)
add_subdirectory(sml)
add_subdirectory(telnet)

#
# List of all SMF libraries
#
set(SMF_LIB_LIST smf_info smf_http_srv smf_protocol_ipt smf_bus_ipt smf_protocol_sml smf_bus_sml smf_protocol_modem smf_protocol_mbus smf_protocol_imega smf_protocol_iec smf_bus_serial smf_protocol_telnet smf_http_srv)
if(${PROJECT_NAME}_SSL_SUPPORT)
    list(APPEND SMF_LIB_LIST smf_https_srv)
endif()
#install(TARGETS ${SMF_LIB_LIST}
#    LIBRARY DESTINATION lib
#    ARCHIVE DESTINATION lib/static)
