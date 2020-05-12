# top level files
set (node_setup)

set (node_setup_cpp

	nodes/setup/src/main.cpp	
	nodes/setup/src/controller.cpp 
	nodes/setup/src/tasks/tables.cpp
)

set (node_setup_h

	nodes/setup/src/controller.h
	nodes/setup/src/tasks/setup_defs.h
	nodes/setup/src/tasks/tables.h
)

set (node_setup_shared
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/show_fs_drives.h

	src/main/include/smf/shared/ctl.h
)


if (UNIX)
	list(APPEND node_setup_shared src/main/include/smf/shared/write_pid.h)
	list(APPEND node_setup_shared nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (node_setup_tasks
	nodes/setup/src/tasks/cluster.h
	nodes/setup/src/tasks/cluster.cpp
	nodes/setup/src/tasks/storage_db.h
	nodes/setup/src/tasks/storage_db.cpp
	nodes/setup/src/tasks/storage_json.h
	nodes/setup/src/tasks/storage_json.cpp
	nodes/setup/src/tasks/storage_xml.h
	nodes/setup/src/tasks/storage_xml.cpp
	nodes/setup/src/tasks/sync.h
	nodes/setup/src/tasks/sync.cpp
)

set (node_setup_schemes

	src/main/include/smf/shared/db_schemes.h
	nodes/shared/db/db_schemes.cpp
)

if(WIN32)

	set (node_setup_service
		nodes/setup/templates/setup_create_service.cmd.in
		nodes/setup/templates/setup_delete_service.cmd.in
		nodes/setup/templates/setup_restart_service.cmd.in
		nodes/setup/templates/setup.windows.cgf.in
	)
 
 	set (node_setup_res
		${CMAKE_CURRENT_BINARY_DIR}/setup.rc 
		src/main/resources/logo.ico
		nodes/setup/templates/setup.exe.manifest
	)

else()

	set (node_setup_service
		nodes/setup/templates/setup.service.in
		nodes/setup/templates/setup.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_setup_tasks})
source_group("service" FILES ${node_setup_service})
source_group("shared" FILES ${node_setup_shared})
source_group("schemes" FILES ${node_setup_schemes})


# define the main program
set (node_setup
  ${node_setup_cpp}
  ${node_setup_h}
  ${node_setup_tasks}
  ${node_setup_service}
  ${node_setup_shared}
  ${node_setup_schemes}
)

if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	set (node_setup_xml
		${PUGIXML_INCLUDE_DIR}/pugixml.hpp
		${PUGIXML_INCLUDE_DIR}/pugixml.cpp
	)
	list(APPEND node_setup ${node_setup_xml})
	source_group("XML" FILES ${node_setup_xml})

endif()

if(WIN32)
	source_group("resources" FILES ${node_setup_res})
	list(APPEND node_setup ${node_setup_res})
endif()
