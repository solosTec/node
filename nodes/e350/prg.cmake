# top level files
set (node_e350)

set (node_e350_cpp

	src/main.cpp	
	src/controller.cpp
	${CMAKE_SOURCE_DIR}/nodes/shared/net/server_stub.cpp
	${CMAKE_SOURCE_DIR}/nodes/shared/net/session_stub.cpp
	src/server.cpp
	src/session.cpp
)

set (node_e350_h

	src/controller.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/cluster/server_stub.h
	src/server.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/cluster/session_stub.h
	src/session.h
)

set (node_e350_shared
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h

)

if (UNIX)
	list(APPEND node_e350_shared ${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/write_pid.h)
	list(APPEND node_e350_shared ${CMAKE_SOURCE_DIR}/nodes/shared/sys/write_pid.cpp)
endif(UNIX)


set (node_e350_tasks
	src/tasks/cluster.h
	src/tasks/cluster.cpp
	${CMAKE_SOURCE_DIR}/nodes/shared/tasks/gatekeeper.h
	${CMAKE_SOURCE_DIR}/nodes/shared/tasks/gatekeeper.cpp
)
	
if(WIN32)

	set (node_e350_service
		templates/e350_create_service.cmd.in
		templates/e350_delete_service.cmd.in
		templates/e350_restart_service.cmd.in
		templates/e350.windows.cgf.in
	)

 	set (node_e350_res
		${CMAKE_BINARY_DIR}/e350.rc 
		${CMAKE_SOURCE_DIR}/src/main/resources/logo.ico
		templates/e350.exe.manifest
	)

else()

	set (node_e350_service
		templates/e350.service.in
		templates/e350.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_e350_tasks})
source_group("service" FILES ${node_e350_service})
source_group("shared" FILES ${node_e350_shared})


# define the main program
set (node_e350
  ${node_e350_cpp}
  ${node_e350_h}
  ${node_e350_shared}
  ${node_e350_tasks}
  ${node_e350_res}
  ${node_e350_service}
)

if(WIN32)
	source_group("resources" FILES ${node_e350_res})
	list(APPEND node_e350 ${node_e350_res})
endif()
