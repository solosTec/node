# top level files
set (node_ipt_segw)

set (node_ipt_segw_cpp

	src/main.cpp	
	src/segw.cpp	
	src/controller.cpp
	src/storage.cpp
	src/storage_global.cpp
	src/cache.cpp
	src/bridge.cpp
	src/router.cpp
	src/lmn.cpp
	src/mbus_decoder.cpp
	src/profiles.cpp

)

set (node_ipt_segw_h

	src/controller.h
	src/segw.h
	src/storage.h
	src/storage_global.h
	src/cache.h
	src/bridge.h
	src/router.h
	src/lmn.h
	src/mbus_decoder.h
	src/profiles.h

)

set (node_ipt_segw_config

	src/cfg_ipt.h
	src/cfg_rs485.h
	src/cfg_wmbus.h
	src/cfg_mbus.h
	src/cfg_server.h
	src/cfg_broker.h
	src/cfg_redirector.h
	src/cfg_gpio.h
	src/cfg_nms.h

	src/cfg_ipt.cpp
	src/cfg_rs485.cpp
	src/cfg_wmbus.cpp
	src/cfg_mbus.cpp
	src/cfg_server.cpp
	src/cfg_broker.cpp
	src/cfg_redirector.cpp
	src/cfg_gpio.cpp	
	src/cfg_nms.cpp

)

set (node_ipt_segw_shared
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h
	${CMAKE_SOURCE_DIR}/nodes/show_fs_drives.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h
	${CMAKE_SOURCE_DIR}/nodes/shared/sys/ctl.cpp
)

if (UNIX)
	list(APPEND node_ipt_segw_shared ${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/write_pid.h)
endif(UNIX)


set (node_ipt_segw_tasks

	src/tasks/gpio.h
	src/tasks/gpio.cpp
	src/tasks/obislog.h
	src/tasks/obislog.cpp
	src/tasks/readout.h
	src/tasks/readout.cpp
	src/tasks/network.h
	src/tasks/network.cpp
	src/tasks/connect.h
	src/tasks/connect.cpp
	src/tasks/lmn_port.h
	src/tasks/lmn_port.cpp
	src/tasks/lmn_status.h
	src/tasks/lmn_status.cpp
	src/tasks/parser_wmbus.h
	src/tasks/parser_wmbus.cpp
	src/tasks/parser_serial.h
	src/tasks/parser_serial.cpp
	src/tasks/parser_CP210x.h
	src/tasks/parser_CP210x.cpp
	src/tasks/push.h
	src/tasks/push.cpp
	src/tasks/limiter.h
	src/tasks/limiter.cpp
	src/tasks/rs485.h
	src/tasks/rs485.cpp
	src/tasks/iec.h
	src/tasks/iec.cpp
	src/tasks/clock.h
	src/tasks/clock.cpp
	src/tasks/broker.h
	src/tasks/broker.cpp
	src/tasks/reflux.h
	src/tasks/reflux.cpp
	src/tasks/rebind.h
	src/tasks/rebind.cpp
)
	
set (node_ipt_sml_server

	src/sml/server.h
	src/sml/server.cpp	
	src/sml/session.h
	src/sml/session.cpp	

)

set (node_ipt_nms_server

	src/nms/server.h
	src/nms/server.cpp	
	src/nms/session.h
	src/nms/session.cpp	
)

set (node_ipt_redirector_server

	src/redirector/server.h
	src/redirector/server.cpp	
	src/redirector/session.h
	src/redirector/session.cpp	
)


set (node_ipt_segw_msg

	src/msg/get_proc_parameter.cpp
	src/msg/get_proc_parameter.h	
	src/msg/get_profile_list.h
	src/msg/get_profile_list.cpp	
	src/msg/get_list.h
	src/msg/get_list.cpp	
	src/msg/set_proc_parameter.h
	src/msg/set_proc_parameter.cpp	
	src/msg/config_ipt.h
	src/msg/config_ipt.cpp	
	src/msg/attention.h
	src/msg/attention.cpp	
	src/msg/config_sensor_params.h
	src/msg/config_sensor_params.cpp
	src/msg/config_data_collector.h
	src/msg/config_data_collector.cpp
	src/msg/config_security.h
	src/msg/config_security.cpp
	src/msg/config_access.h
	src/msg/config_access.cpp
	src/msg/config_iec.h
	src/msg/config_iec.cpp
	src/msg/config_broker.h
	src/msg/config_broker.cpp
	src/msg/config_customer_if.h
	src/msg/config_customer_if.cpp
)

if(WIN32)

	set (node_ipt_segw_service
		templates/segw_create_service.cmd.in
		templates/segw_delete_service.cmd.in
		templates/segw_restart_service.cmd.in
		templates/segw.windows.cgf.in
	)
 
	set (node_ipt_segw_manifest
		templates/segw.exe.manifest
	)

	set (node_ipt_segw_res
		${CMAKE_BINARY_DIR}/segw.rc 
	)

else()

	set (node_ipt_segw_service
		templates/segw.service.in
		templates/segw.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_ipt_segw_tasks})
source_group("service" FILES ${node_ipt_segw_service})
source_group("shared" FILES ${node_ipt_segw_shared})
source_group("server" FILES ${node_ipt_sml_server})
source_group("nms" FILES ${node_ipt_nms_server})
source_group("redirector" FILES ${node_ipt_redirector_server})
source_group("messaging" FILES ${node_ipt_segw_msg})
source_group("config" FILES  ${node_ipt_segw_config})


# define the main program
set (node_ipt_segw
  ${node_ipt_segw_cpp}
  ${node_ipt_segw_h}
  ${node_ipt_segw_tasks}
  ${node_ipt_segw_service}
  ${node_ipt_segw_shared}
  ${node_ipt_sml_server}
  ${node_ipt_nms_server}
  ${node_ipt_redirector_server}
  ${node_ipt_segw_msg}
  ${node_ipt_segw_config}
)

if(WIN32)
	source_group("manifest" FILES ${node_ipt_segw_manifest})
	list(APPEND node_ipt_segw ${node_ipt_segw_manifest})
	source_group("resources" FILES ${node_ipt_segw_res})
	list(APPEND node_ipt_segw ${node_ipt_segw_res})
endif()

