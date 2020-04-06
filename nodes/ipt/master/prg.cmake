# top level files
set (node_ipt_master)
set (node_ipts_master)

set (node_ipt_master_cpp

	nodes/ipt/master/ipt/src/main.cpp	
	nodes/ipt/master/ipt/src/controller.cpp
	nodes/ipt/master/ipt/src/server.cpp
	nodes/ipt/master/ipt/src/session.cpp
	nodes/ipt/master/ipt/src/session_state.cpp
	nodes/ipt/master/ipt/src/proxy_data.cpp
	nodes/ipt/master/ipt/src/proxy_comm.cpp
	nodes/ipt/master/ipt/src/config_cache.cpp

	nodes/shared/net/server_stub.cpp
	nodes/shared/net/session_stub.cpp
)

set (node_ipt_master_h

	nodes/ipt/master/ipt/src/controller.h
	nodes/ipt/master/ipt/src/server.h
	nodes/ipt/master/ipt/src/session.h
	nodes/ipt/master/ipt/src/session_state.h
	nodes/ipt/master/ipt/src/proxy_data.h
	nodes/ipt/master/ipt/src/proxy_comm.h
	nodes/ipt/master/ipt/src/config_cache.h

	src/main/include/smf/cluster/server_stub.h
	src/main/include/smf/cluster/session_stub.h
)

set (node_ipt_master_shared
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h

	src/main/include/smf/shared/ctl.h
)

set (node_ipt_master_tasks
	nodes/shared/tasks/gatekeeper.h
	nodes/shared/tasks/gatekeeper.cpp
	nodes/ipt/master/ipt/src/tasks/cluster.h
	nodes/ipt/master/ipt/src/tasks/cluster.cpp
	nodes/ipt/master/ipt/src/tasks/open_connection.h
	nodes/ipt/master/ipt/src/tasks/open_connection.cpp
	nodes/ipt/master/ipt/src/tasks/close_connection.h
	nodes/ipt/master/ipt/src/tasks/close_connection.cpp
	nodes/ipt/master/ipt/src/tasks/watchdog.h
	nodes/ipt/master/ipt/src/tasks/watchdog.cpp
	nodes/ipt/master/ipt/src/tasks/gateway_proxy.h
	nodes/ipt/master/ipt/src/tasks/gateway_proxy.cpp
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
source_group("shared" FILES ${node_ipt_master_shared})


# define the main program
set (node_ipt_master
  ${node_ipt_master_cpp}
  ${node_ipt_master_h}
  ${node_ipt_master_tasks}
  ${node_ipt_master_service}
  ${node_ipt_master_shared}
)

if(WIN32)
	source_group("resources" FILES ${node_ipt_master_res})
	list(APPEND node_ipt_master ${node_ipt_master_res})
endif()

# --------------------------------------------------------------------+
set (node_ipts_master_cpp

	nodes/ipt/master/ipts/src/main.cpp	
	nodes/ipt/master/ipts/src/controller.cpp
	nodes/ipt/master/ipts/src/server.cpp
	nodes/ipt/master/ipts/src/session.cpp
	nodes/ipt/master/ipts/src/session_state.cpp
	nodes/ipt/master/ipts/src/proxy_data.cpp
	nodes/ipt/master/ipts/src/proxy_comm.cpp

	nodes/shared/net/server_stub.cpp
	nodes/shared/net/session_stub.cpp
)

set (node_ipts_master_h

	nodes/ipt/master/ipts/src/controller.h
	nodes/ipt/master/ipts/src/server.h
	nodes/ipt/master/ipts/src/session.h
	nodes/ipt/master/ipts/src/session_state.h
	nodes/ipt/master/ipts/src/proxy_data.h
	nodes/ipt/master/ipts/src/proxy_comm.h

	src/main/include/smf/cluster/server_stub.h
	src/main/include/smf/cluster/session_stub.h
)

set (node_ipts_master_shared
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h

	src/main/include/smf/shared/ctl.h
)

set (node_ipts_master_tasks
	nodes/shared/tasks/gatekeeper.h
	nodes/shared/tasks/gatekeeper.cpp
	nodes/ipt/master/ipts/src/tasks/cluster.h
	nodes/ipt/master/ipts/src/tasks/cluster.cpp
	nodes/ipt/master/ipts/src/tasks/open_connection.h
	nodes/ipt/master/ipts/src/tasks/open_connection.cpp
	nodes/ipt/master/ipts/src/tasks/close_connection.h
	nodes/ipt/master/ipts/src/tasks/close_connection.cpp
	nodes/ipt/master/ipts/src/tasks/watchdog.h
	nodes/ipt/master/ipts/src/tasks/watchdog.cpp
	nodes/ipt/master/ipts/src/tasks/gateway_proxy.h
	nodes/ipt/master/ipts/src/tasks/gateway_proxy.cpp
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

# define the main program
set (node_ipts_master
  ${node_ipts_master_cpp}
  ${node_ipts_master_h}
  ${node_ipts_master_tasks}
  ${node_ipts_master_service}
  ${node_ipts_master_shared}
)

if(WIN32)
#	source_group("resources" FILES ${node_ipts_master_res})
	list(APPEND node_ipts_master ${node_ipts_master_res})
endif()
