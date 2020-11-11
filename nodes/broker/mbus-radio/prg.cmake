# top level files
set (node_mbus_radio)

set (node_mbus_radio_cpp

	src/main.cpp	
	src/controller.cpp
	src/sync_db.cpp
	src/server.cpp
	src/session.cpp
	src/cache.cpp

)

set (node_mbus_radio_h

	src/controller.h
	src/sync_db.h
	src/server.h
	src/session.h
	src/cache.h

)

set (node_shared
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/protocols.h
	${CMAKE_SOURCE_DIR}/nodes/shared/protocols.cpp

)


if (UNIX)
	list(APPEND node_mbus_radio_shared src/main/include/smf/shared/write_pid.h)
	list(APPEND node_mbus_radio_shared nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (node_mbus_radio_tasks
	src/tasks/cluster.h
	src/tasks/cluster.cpp
)

set (node_schemes

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/db_schemes.h
	${CMAKE_SOURCE_DIR}/nodes/shared/db/db_schemes.cpp
)
	
if(WIN32)

	set (node_mbus_radio_service
		templates/mbus_radio_create_service.cmd.in
		templates/mbus_radio_delete_service.cmd.in
		templates/mbus_radio_restart_service.cmd.in
		templates/mbus_radio.windows.cgf.in
	)
 
else()

	set (node_mbus_radio_service
		templates/mbus_radio.service.in
		templates/mbus_radio.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_mbus_radio_tasks})
source_group("resources" FILES ${node_mbus_radio_res})
source_group("service" FILES ${node_mbus_radio_service})
source_group("shared" FILES ${node_shared})
source_group("schemes" FILES ${node_schemes})


# define the main program
set (node_mbus_radio
  ${node_mbus_radio_cpp}
  ${node_mbus_radio_h}
  ${node_mbus_radio_tasks}
  ${node_mbus_radio_service}
  ${node_shared}
  ${node_schemes}
)


