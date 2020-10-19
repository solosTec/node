# top level files
set (node_ipt_master)
set (node_ipts_master)

set (node_ipt_master_cpp

	ipt/src/main.cpp	
	ipt/src/controller.cpp
	ipt/src/server.cpp
	ipt/src/session.cpp
	ipt/src/session_state.cpp
	ipt/src/proxy_comm.cpp

	${CMAKE_SOURCE_DIR}/nodes/shared/net/server_stub.cpp
	${CMAKE_SOURCE_DIR}/nodes/shared/net/session_stub.cpp
)

set (node_ipt_master_h

	ipt/src/controller.h
	ipt/src/server.h
	ipt/src/session.h
	ipt/src/session_state.h
	ipt/src/proxy_comm.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/cluster/server_stub.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/cluster/session_stub.h
)

set (node_ipt_master_shared
	shared/proxy_data.h
	shared/proxy_data.cpp
	shared/config_cache.h
	shared/config_cache.cpp
	shared/transformer.h
	shared/transformer.cpp
)

set (node_shared
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h
)

set (node_ipt_master_tasks
	${CMAKE_SOURCE_DIR}/nodes/shared/tasks/gatekeeper.h
	${CMAKE_SOURCE_DIR}/nodes/shared/tasks/gatekeeper.cpp
	ipt/src/tasks/cluster.h
	ipt/src/tasks/cluster.cpp
	ipt/src/tasks/open_connection.h
	ipt/src/tasks/open_connection.cpp
	ipt/src/tasks/close_connection.h
	ipt/src/tasks/close_connection.cpp

	shared/tasks/gateway_proxy.h
	shared/tasks/gateway_proxy.cpp
	shared/tasks/watchdog.h
	shared/tasks/watchdog.cpp
)
	
if(WIN32)

	set (node_ipt_master_service
		templates/ipt_create_service.cmd.in
		templates/ipt_delete_service.cmd.in
		templates/ipt_restart_service.cmd.in
		templates/ipt.windows.cgf.in
	)

	set (node_ipt_master_res
		${CMAKE_BINARY_DIR}/ipt.rc 
		${CMAKE_SOURCE_DIR}/src/main/resources/logo.ico
		templates/ipt.exe.manifest
	)
 
else()

	set (node_ipt_master_service
		templates/ipt.service.in
		templates/ipt.linux.cgf.in
)

endif()

source_group("tasks" FILES ${node_ipt_master_tasks})
source_group("service" FILES ${node_ipt_master_service})
source_group("shared" FILES ${node_shared})
source_group("ipt" FILES ${node_ipt_master_shared})


# define the main program
set (node_ipt_master
  ${node_ipt_master_cpp}
  ${node_ipt_master_h}
  ${node_ipt_master_tasks}
  ${node_ipt_master_service}
  ${node_ipt_master_shared}
  ${node_shared}
)

if(WIN32)
	source_group("resources" FILES ${node_ipt_master_res})
	list(APPEND node_ipt_master ${node_ipt_master_res})
endif()

# --------------------------------------------------------------------+
set (node_ipts_master_cpp

	ipts/src/main.cpp	
	ipts/src/controller.cpp
	ipts/src/server.cpp
	ipts/src/session.cpp
	ipts/src/session_state.cpp
	ipts/src/proxy_comm.cpp

	${CMAKE_SOURCE_DIR}/nodes/shared/net/server_stub.cpp
	${CMAKE_SOURCE_DIR}/nodes/shared/net/session_stub.cpp
)

set (node_ipts_master_h

	ipts/src/controller.h
	ipts/src/server.h
	ipts/src/session.h
	ipts/src/session_state.h
	ipts/src/proxy_comm.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/cluster/server_stub.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/cluster/session_stub.h
)

set (node_ipts_master_tasks
	${CMAKE_SOURCE_DIR}/nodes/shared/tasks/gatekeeper.h
	${CMAKE_SOURCE_DIR}/nodes/shared/tasks/gatekeeper.cpp
	ipts/src/tasks/cluster.h
	ipts/src/tasks/cluster.cpp
	ipts/src/tasks/open_connection.h
	ipts/src/tasks/open_connection.cpp
	ipts/src/tasks/close_connection.h
	ipts/src/tasks/close_connection.cpp

	shared/tasks/gateway_proxy.h
	shared/tasks/gateway_proxy.cpp
	shared/tasks/watchdog.h
	shared/tasks/watchdog.cpp
)
	
if(WIN32)

	set (node_ipt_master_service
		templates/ipt_create_service.cmd.in
		templates/ipt_delete_service.cmd.in
		templates/ipt_restart_service.cmd.in
		templates/ipt.windows.cgf.in
	)

	set (node_ipt_master_res
		${CMAKE_CURRENT_BINARY_DIR}/ipt.rc 
		src/main/resources/logo.ico
		templates/ipt.exe.manifest
	)

 
else()

	set (node_ipt_master_service
		templates/ipt.service.in
		templates/ipt.linux.cgf.in
)

endif()

source_group("taskss" FILES ${node_ipts_master_tasks})

# define the main program
set (node_ipts_master
  ${node_ipts_master_cpp}
  ${node_ipts_master_h}
  ${node_ipts_master_tasks}
  ${node_ipts_master_service}
  ${node_ipt_master_shared}
  ${node_shared}
)

if(WIN32)
#	source_group("resources" FILES ${node_ipts_master_res})
	list(APPEND node_ipts_master ${node_ipts_master_res})
endif()
