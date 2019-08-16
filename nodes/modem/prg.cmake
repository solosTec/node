# top level files
set (node_modem)

set (node_modem_cpp

	nodes/modem/src/main.cpp	
	nodes/modem/src/controller.cpp
	nodes/shared/net/server_stub.cpp
	nodes/shared/net/session_stub.cpp
	nodes/modem/src/server.cpp
	nodes/modem/src/session.cpp
	nodes/modem/src/session_state.cpp
)

set (node_modem_h

	nodes/modem/src/controller.h
	src/main/include/smf/cluster/server_stub.h
	nodes/modem/src/server.h
	src/main/include/smf/cluster/session_stub.h
	nodes/modem/src/session.h
	nodes/modem/src/session_state.h
)

set (node_modem_shared
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp

	src/main/include/smf/shared/ctl.h
	nodes/shared/sys/ctl.cpp
)

if (UNIX)
	list(APPEND node_modem_shared src/main/include/smf/shared/write_pid.h)
	list(APPEND node_modem_shared nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (node_modem_tasks
	nodes/modem/src/tasks/cluster.h
	nodes/modem/src/tasks/cluster.cpp
#	nodes/modem/src/tasks/gatekeeper.h
#	nodes/modem/src/tasks/gatekeeper.cpp
	nodes/shared/tasks/gatekeeper.h
	nodes/shared/tasks/gatekeeper.cpp
)
	
if(WIN32)

	set (node_modem_service
		nodes/modem/templates/modem_create_service.cmd.in
		nodes/modem/templates/modem_delete_service.cmd.in
		nodes/modem/templates/modem_restart_service.cmd.in
		nodes/modem/templates/modem.windows.cgf.in
	)
 
else()

	set (node_modem_service
		nodes/modem/templates/modem.service.in
		nodes/modem/templates/modem.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_modem_tasks})
source_group("service" FILES ${node_modem_service})
source_group("shared" FILES ${node_modem_shared})


# define the main program
set (node_modem
  ${node_modem_cpp}
  ${node_modem_h}
  ${node_modem_tasks}
  ${node_modem_service}
  ${node_modem_shared}
)

