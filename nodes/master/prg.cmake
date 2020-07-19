# top level files
set (node_master)

set (node_master_cpp

	src/main.cpp	
	src/controller.cpp
	src/server.cpp
	src/connection.cpp
	src/db.cpp
	src/session.cpp
	src/client.cpp
	src/cluster.cpp
)

set (node_master_h

	src/controller.h
	src/server.h
	src/connection.h
	src/db.h
	src/session.h
	src/client.h
	src/cluster.h
)

set (node_master_tasks
	src/tasks/watchdog.h
	src/tasks/watchdog.cpp	
)

set (node_master_shared

	${CMAKE_SOURCE_DIR}/src/main/include/smf/cluster/generator.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/cluster/serializer.h
	${CMAKE_SOURCE_DIR}/lib/shared/src/generator.cpp
	${CMAKE_SOURCE_DIR}/lib/shared/src/serializer.cpp

	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h
)

if (UNIX)
	list(APPEND node_master_shared src/main/include/smf/shared/write_pid.h)
	list(APPEND node_master_shared nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (node_master_schemes

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/db_schemes.h
	${CMAKE_SOURCE_DIR}/nodes/shared/db/db_schemes.cpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/db_cfg.h
	${CMAKE_SOURCE_DIR}/nodes/shared/db/db_cfg.cpp
)

if(WIN32)

	set (node_master_service
		templates/master_create_service.cmd.in
		templates/master_delete_service.cmd.in
		templates/master_restart_service.cmd.in
		templates/master.windows.cgf.in
	)

	set (node_master_res
		${CMAKE_BINARY_DIR}/master.rc 
		${CMAKE_SOURCE_DIR}/src/main/resources/logo.ico
		templates/master.exe.manifest
	)

 
else()

	set (node_master_service
		templates/master.service.in
		templates/master.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_master_tasks})
source_group("service" FILES ${node_master_service})
source_group("shared" FILES ${node_master_shared})
source_group("schemes" FILES ${node_master_schemes})


# define the main program
set (node_master
  ${node_master_cpp}
  ${node_master_h}
  ${node_master_tasks}
  ${node_master_res}
  ${node_master_service}
  ${node_master_shared}
  ${node_master_schemes}
)

if(WIN32)
	source_group("resources" FILES ${node_master_res})
	list(APPEND node_master ${node_master_res})
endif()
