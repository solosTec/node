# top level files
set (node_ipt_master)

set (node_ipt_master_cpp

	nodes/ipt/master/src/main.cpp	
	nodes/ipt/master/src/controller.cpp
	nodes/shared/net/server_stub.cpp
	nodes/shared/net/session_stub.cpp
	nodes/ipt/master/src/server.cpp
	nodes/ipt/master/src/session.cpp
	nodes/ipt/master/src/session_state.cpp
	nodes/ipt/master/src/proxy_data.cpp
	nodes/ipt/master/src/proxy_comm.cpp
)

set (node_ipt_master_h

	nodes/ipt/master/src/controller.h
	src/main/include/smf/cluster/server_stub.h
	nodes/ipt/master/src/server.h
	src/main/include/smf/cluster/session_stub.h
	nodes/ipt/master/src/session.h
	nodes/ipt/master/src/session_state.h
	nodes/ipt/master/src/proxy_data.h
	nodes/ipt/master/src/proxy_comm.h
)

set (node_ipt_master_info
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/write_pid.h

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/write_pid.cpp
)

set (node_ipt_master_tasks
	nodes/shared/tasks/gatekeeper.h
	nodes/shared/tasks/gatekeeper.cpp
	nodes/ipt/master/src/tasks/cluster.h
	nodes/ipt/master/src/tasks/cluster.cpp
	nodes/ipt/master/src/tasks/open_connection.h
	nodes/ipt/master/src/tasks/open_connection.cpp
	nodes/ipt/master/src/tasks/close_connection.h
	nodes/ipt/master/src/tasks/close_connection.cpp
	nodes/ipt/master/src/tasks/watchdog.h
	nodes/ipt/master/src/tasks/watchdog.cpp
	nodes/ipt/master/src/tasks/gateway_proxy.h
	nodes/ipt/master/src/tasks/gateway_proxy.cpp
)
	
if(WIN32)

	set (node_ipt_master_service
		nodes/ipt/master/templates/ipt_create_service.cmd.in
		nodes/ipt/master/templates/ipt_delete_service.cmd.in
		nodes/ipt/master/templates/ipt_restart_service.cmd.in
		nodes/ipt/master/templates/ipt.windows.cgf.in
	)

	set (node_ipt_master_res
		${CMAKE_CURRENT_BINARY_DIR}/ipt.rc 
		src/main/resources/logo.ico
		nodes/ipt/master/templates/ipt.exe.manifest
	)

 
else()

	set (node_ipt_master_service
		nodes/ipt/master/templates/ipt.service.in
		nodes/ipt/master/templates/ipt.linux.cgf.in
)

endif()

source_group("tasks" FILES ${node_ipt_master_tasks})
source_group("service" FILES ${node_ipt_master_service})
source_group("info" FILES ${node_ipt_master_info})


# define the main program
set (node_ipt_master
  ${node_ipt_master_cpp}
  ${node_ipt_master_h}
  ${node_ipt_master_tasks}
  ${node_ipt_master_service}
  ${node_ipt_master_info}
)

if(WIN32)
	source_group("resources" FILES ${node_ipt_master_res})
	list(APPEND node_ipt_master ${node_ipt_master_res})
endif()
