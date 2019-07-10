# top level files
set (node_master)

set (node_master_cpp

	nodes/master/src/main.cpp	
	nodes/master/src/controller.cpp
	nodes/master/src/server.cpp
	nodes/master/src/connection.cpp
	nodes/master/src/db.cpp
	nodes/master/src/session.cpp
	nodes/master/src/client.cpp
	nodes/master/src/cluster.cpp
)

set (node_master_h

	nodes/master/src/controller.h
	nodes/master/src/server.h
	nodes/master/src/connection.h
	nodes/master/src/db.h
	nodes/master/src/session.h
	nodes/master/src/client.h
	nodes/master/src/cluster.h
)

set (node_master_info
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

set (node_master_tasks
	nodes/master/src/tasks/watchdog.h
	nodes/master/src/tasks/watchdog.cpp	
)

set (node_master_shared

	src/main/include/smf/cluster/generator.h
	src/main/include/smf/cluster/serializer.h
	lib/shared/src/generator.cpp
	lib/shared/src/serializer.cpp
)


set (node_master_schemes

	src/main/include/smf/shared/db_schemes.h
	nodes/shared/db/db_schemes.cpp
)

if(WIN32)

	set (node_master_service
		nodes/master/templates/master_create_service.cmd.in
		nodes/master/templates/master_delete_service.cmd.in
		nodes/master/templates/master_restart_service.cmd.in
		nodes/master/templates/master.windows.cgf.in
	)

	set (node_master_res
		${CMAKE_CURRENT_BINARY_DIR}/master.rc 
		src/main/resources/logo.ico
		nodes/master/templates/master.exe.manifest
	)

 
else()

	set (node_master_service
		nodes/master/templates/master.service.in
		nodes/master/templates/master.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_master_tasks})
source_group("service" FILES ${node_master_service})
source_group("info" FILES ${node_master_info})
source_group("shared" FILES ${node_master_shared})
source_group("schemes" FILES ${node_master_schemes})


# define the main program
set (node_master
  ${node_master_cpp}
  ${node_master_h}
  ${node_master_tasks}
  ${node_master_res}
  ${node_master_service}
  ${node_master_info}
  ${node_master_shared}
  ${node_master_schemes}
)

if(WIN32)
	source_group("resources" FILES ${node_master_res})
	list(APPEND node_master ${node_master_res})
endif()
