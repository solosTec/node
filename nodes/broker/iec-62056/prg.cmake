# top level files
set (node_iec_62056)

set (node_iec_62056_cpp

	src/main.cpp	
	src/controller.cpp
	src/sync_db.cpp
	src/server.cpp
	src/session.cpp
)

set (node_iec_62056_h

	src/controller.h
	src/sync_db.h
	src/server.h
	src/session.h

)

set (node_shared
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h
)

set (node_schemes

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/db_schemes.h
	${CMAKE_SOURCE_DIR}/nodes/shared/db/db_schemes.cpp
)


if (UNIX)
	list(APPEND node_iec_62056_shared src/main/include/smf/shared/write_pid.h)
	list(APPEND node_iec_62056_shared nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (node_iec_62056_tasks
	src/tasks/cluster.h
	src/tasks/cluster.cpp
)

	
if(WIN32)

	set (node_iec_62056_service
		templates/iec_62056_create_service.cmd.in
		templates/iec_62056_delete_service.cmd.in
		templates/iec_62056_restart_service.cmd.in
		templates/iec_62056.windows.cgf.in
	)
 
else()

	set (node_iec_62056_service
		templates/iec_62056.service.in
		templates/iec_62056.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_iec_62056_tasks})
source_group("resources" FILES ${node_iec_62056_res})
source_group("service" FILES ${node_iec_62056_service})
source_group("shared" FILES ${node_shared})
source_group("schemes" FILES ${node_schemes})


# define the main program
set (node_iec_62056
  ${node_iec_62056_cpp}
  ${node_iec_62056_h}
  ${node_iec_62056_tasks}
  ${node_iec_62056_service}
  ${node_shared}
  ${node_schemes}
)


