# top level files
set (node_modem)

set (node_modem_cpp

	src/main.cpp	
	src/controller.cpp
	${CMAKE_SOURCE_DIR}/nodes/shared/net/server_stub.cpp
	${CMAKE_SOURCE_DIR}/nodes/shared/net/session_stub.cpp
	src/server.cpp
	src/session.cpp
	src/session_state.cpp
)

set (node_modem_h

	src/controller.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/cluster/server_stub.h
	src/server.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/cluster/session_stub.h
	src/session.h
	src/session_state.h
)

set (node_modem_shared
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h
)

if (UNIX)
	list(APPEND node_modem_shared ${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/write_pid.h)
endif(UNIX)

set (node_modem_tasks
	src/tasks/cluster.h
	src/tasks/cluster.cpp
	${CMAKE_SOURCE_DIR}/nodes/shared/tasks/gatekeeper.h
	${CMAKE_SOURCE_DIR}/nodes/shared/tasks/gatekeeper.cpp
)
	
if(WIN32)

	set (node_modem_service
		templates/modem_create_service.cmd.in
		templates/modem_delete_service.cmd.in
		templates/modem_restart_service.cmd.in
		templates/modem.windows.cgf.in
	)
 
else()

	set (node_modem_service
		templates/modem.service.in
		templates/modem.linux.cgf.in
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

