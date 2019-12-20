# top level files
set (node_ipt_segw)

set (node_ipt_segw_cpp

	nodes/ipt/segw/src/main.cpp	
	nodes/ipt/segw/src/segw.cpp	
	nodes/ipt/segw/src/controller.cpp
	nodes/ipt/segw/src/storage.cpp
	nodes/ipt/segw/src/cache.cpp
	nodes/ipt/segw/src/cfg_ipt.cpp
	nodes/ipt/segw/src/bridge.cpp
	nodes/ipt/segw/src/router.cpp
	nodes/ipt/segw/src/lmn.cpp
	nodes/ipt/segw/src/decoder.cpp

)

set (node_ipt_segw_h

	nodes/ipt/segw/src/controller.h
	nodes/ipt/segw/src/segw.h
	nodes/ipt/segw/src/storage.h
	nodes/ipt/segw/src/cache.h
	nodes/ipt/segw/src/cfg_ipt.h
	nodes/ipt/segw/src/bridge.h
	nodes/ipt/segw/src/router.h
	nodes/ipt/segw/src/lmn.h
	nodes/ipt/segw/src/decoder.h

)

set (node_ipt_segw_shared
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/show_fs_drives.h

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/show_fs_drives.cpp

	src/main/include/smf/shared/ctl.h
	nodes/shared/sys/ctl.cpp
)

if (UNIX)
	list(APPEND node_ipt_segw_shared src/main/include/smf/shared/write_pid.h)
	list(APPEND node_ipt_segw_shared nodes/shared/sys/write_pid.cpp)
endif(UNIX)


set (node_ipt_segw_tasks

	nodes/ipt/segw/src/tasks/gpio.h
	nodes/ipt/segw/src/tasks/gpio.cpp
	nodes/ipt/segw/src/tasks/obislog.h
	nodes/ipt/segw/src/tasks/obislog.cpp
	nodes/ipt/segw/src/tasks/readout.h
	nodes/ipt/segw/src/tasks/readout.cpp
	nodes/ipt/segw/src/tasks/network.h
	nodes/ipt/segw/src/tasks/network.cpp
	nodes/ipt/segw/src/tasks/lmn_port.h
	nodes/ipt/segw/src/tasks/lmn_port.cpp
	nodes/ipt/segw/src/tasks/parser_wmbus.h
	nodes/ipt/segw/src/tasks/parser_wmbus.cpp
	nodes/ipt/segw/src/tasks/parser_serial.h
	nodes/ipt/segw/src/tasks/parser_serial.cpp
	nodes/ipt/segw/src/tasks/parser_CP210x.h
	nodes/ipt/segw/src/tasks/parser_CP210x.cpp
)
	
set (node_ipt_segw_server

	nodes/ipt/segw/src/server/server.h
	nodes/ipt/segw/src/server/server.cpp	
	nodes/ipt/segw/src/server/connection.h
	nodes/ipt/segw/src/server/connection.cpp	
	nodes/ipt/segw/src/server/session.h
	nodes/ipt/segw/src/server/session.cpp	

)

set (node_ipt_segw_msg

	nodes/ipt/segw/src/msg/get_proc_parameter.cpp
	nodes/ipt/segw/src/msg/get_proc_parameter.h	
	nodes/ipt/segw/src/msg/get_profile_list.h
	nodes/ipt/segw/src/msg/get_profile_list.cpp	
	nodes/ipt/segw/src/msg/set_proc_parameter.h
	nodes/ipt/segw/src/msg/set_proc_parameter.cpp	
	nodes/ipt/segw/src/msg/config_ipt.h
	nodes/ipt/segw/src/msg/config_ipt.cpp	
	nodes/ipt/segw/src/msg/attention.h
	nodes/ipt/segw/src/msg/attention.cpp	

)

if(WIN32)

	set (node_ipt_segw_service
		nodes/ipt/segw/templates/segw_create_service.cmd.in
		nodes/ipt/segw/templates/segw_delete_service.cmd.in
		nodes/ipt/segw/templates/segw_restart_service.cmd.in
		nodes/ipt/segw/templates/segw.windows.cgf.in
	)
 
	set (node_ipt_segw_manifest
		nodes/ipt/segw/templates/segw.exe.manifest
	)

	set (node_ipt_segw_res
		${CMAKE_CURRENT_BINARY_DIR}/segw.rc 
	)

else()

	set (node_ipt_segw_service
		nodes/ipt/segw/templates/segw.service.in
		nodes/ipt/segw/templates/segw.linux.cgf.in
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

