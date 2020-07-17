# top level files
set (node_ipt_segw)
set (node_segw_root nodes/ipt/segw)

set (node_ipt_segw_cpp

	${node_segw_root}/src/main.cpp	
	${node_segw_root}/src/segw.cpp	
	${node_segw_root}/src/controller.cpp
	${node_segw_root}/src/storage.cpp
	${node_segw_root}/src/storage_global.cpp
	${node_segw_root}/src/cache.cpp
	${node_segw_root}/src/cfg_ipt.cpp
	${node_segw_root}/src/cfg_rs485.cpp
	${node_segw_root}/src/cfg_wmbus.cpp
	${node_segw_root}/src/cfg_mbus.cpp
	${node_segw_root}/src/cfg_broker.cpp
	${node_segw_root}/src/bridge.cpp
	${node_segw_root}/src/router.cpp
	${node_segw_root}/src/lmn.cpp
	${node_segw_root}/src/decoder.cpp
	${node_segw_root}/src/profiles.cpp

)

set (node_ipt_segw_h

	${node_segw_root}/src/controller.h
	${node_segw_root}/src/segw.h
	${node_segw_root}/src/storage.h
	${node_segw_root}/src/storage_global.h
	${node_segw_root}/src/cache.h
	${node_segw_root}/src/cfg_ipt.h
	${node_segw_root}/src/cfg_rs485.h
	${node_segw_root}/src/cfg_wmbus.h
	${node_segw_root}/src/cfg_mbus.h
	${node_segw_root}/src/cfg_broker.h
	${node_segw_root}/src/bridge.h
	${node_segw_root}/src/router.h
	${node_segw_root}/src/lmn.h
	${node_segw_root}/src/decoder.h
	${node_segw_root}/src/profiles.h

)

set (node_ipt_segw_shared
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/show_fs_drives.h

	src/main/include/smf/shared/ctl.h
	nodes/shared/sys/ctl.cpp
)

if (UNIX)
	list(APPEND node_ipt_segw_shared src/main/include/smf/shared/write_pid.h)
endif(UNIX)


set (node_ipt_segw_tasks

	${node_segw_root}/src/tasks/gpio.h
	${node_segw_root}/src/tasks/gpio.cpp
	${node_segw_root}/src/tasks/obislog.h
	${node_segw_root}/src/tasks/obislog.cpp
	${node_segw_root}/src/tasks/readout.h
	${node_segw_root}/src/tasks/readout.cpp
	${node_segw_root}/src/tasks/network.h
	${node_segw_root}/src/tasks/network.cpp
	${node_segw_root}/src/tasks/connect.h
	${node_segw_root}/src/tasks/connect.cpp
	${node_segw_root}/src/tasks/lmn_port.h
	${node_segw_root}/src/tasks/lmn_port.cpp
	${node_segw_root}/src/tasks/parser_wmbus.h
	${node_segw_root}/src/tasks/parser_wmbus.cpp
	${node_segw_root}/src/tasks/parser_serial.h
	${node_segw_root}/src/tasks/parser_serial.cpp
	${node_segw_root}/src/tasks/parser_CP210x.h
	${node_segw_root}/src/tasks/parser_CP210x.cpp
	${node_segw_root}/src/tasks/push.h
	${node_segw_root}/src/tasks/push.cpp
	${node_segw_root}/src/tasks/limiter.h
	${node_segw_root}/src/tasks/limiter.cpp
	${node_segw_root}/src/tasks/rs485.h
	${node_segw_root}/src/tasks/rs485.cpp
	${node_segw_root}/src/tasks/clock.h
	${node_segw_root}/src/tasks/clock.cpp
	${node_segw_root}/src/tasks/broker_wmbus.h
	${node_segw_root}/src/tasks/broker_wmbus.cpp
)
	
set (node_ipt_segw_server

	${node_segw_root}/src/server/server.h
	${node_segw_root}/src/server/server.cpp	
	${node_segw_root}/src/server/connection.h
	${node_segw_root}/src/server/connection.cpp	
	${node_segw_root}/src/server/session.h
	${node_segw_root}/src/server/session.cpp	

)

set (node_ipt_segw_msg

	${node_segw_root}/src/msg/get_proc_parameter.cpp
	${node_segw_root}/src/msg/get_proc_parameter.h	
	${node_segw_root}/src/msg/get_profile_list.h
	${node_segw_root}/src/msg/get_profile_list.cpp	
	${node_segw_root}/src/msg/get_list.h
	${node_segw_root}/src/msg/get_list.cpp	
	${node_segw_root}/src/msg/set_proc_parameter.h
	${node_segw_root}/src/msg/set_proc_parameter.cpp	
	${node_segw_root}/src/msg/config_ipt.h
	${node_segw_root}/src/msg/config_ipt.cpp	
	${node_segw_root}/src/msg/attention.h
	${node_segw_root}/src/msg/attention.cpp	
	${node_segw_root}/src/msg/config_sensor_params.h
	${node_segw_root}/src/msg/config_sensor_params.cpp
	${node_segw_root}/src/msg/config_data_collector.h
	${node_segw_root}/src/msg/config_data_collector.cpp
	${node_segw_root}/src/msg/config_security.h
	${node_segw_root}/src/msg/config_security.cpp
	${node_segw_root}/src/msg/config_access.h
	${node_segw_root}/src/msg/config_access.cpp
	${node_segw_root}/src/msg/config_iec.h
	${node_segw_root}/src/msg/config_iec.cpp
	${node_segw_root}/src/msg/config_broker.h
	${node_segw_root}/src/msg/config_broker.cpp
)

if(WIN32)

	set (node_ipt_segw_service
		${node_segw_root}/templates/segw_create_service.cmd.in
		${node_segw_root}/templates/segw_delete_service.cmd.in
		${node_segw_root}/templates/segw_restart_service.cmd.in
		${node_segw_root}/templates/segw.windows.cgf.in
	)
 
	set (node_ipt_segw_manifest
		${node_segw_root}/templates/segw.exe.manifest
	)

	set (node_ipt_segw_res
		${CMAKE_CURRENT_BINARY_DIR}/segw.rc 
	)

else()

	set (node_ipt_segw_service
		${node_segw_root}/templates/segw.service.in
		${node_segw_root}/templates/segw.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_ipt_segw_tasks})
source_group("service" FILES ${node_ipt_segw_service})
source_group("shared" FILES ${node_ipt_segw_shared})
source_group("server" FILES ${node_ipt_segw_server})
source_group("messaging" FILES ${node_ipt_segw_msg})


# define the main program
set (node_ipt_segw
  ${node_ipt_segw_cpp}
  ${node_ipt_segw_h}
  ${node_ipt_segw_tasks}
  ${node_ipt_segw_service}
  ${node_ipt_segw_shared}
  ${node_ipt_segw_server}
  ${node_ipt_segw_msg}
)

if(WIN32)
	source_group("manifest" FILES ${node_ipt_segw_manifest})
	list(APPEND node_ipt_segw ${node_ipt_segw_manifest})
	source_group("resources" FILES ${node_ipt_segw_res})
	list(APPEND node_ipt_segw ${node_ipt_segw_res})
endif()

