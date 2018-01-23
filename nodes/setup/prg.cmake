# top level files
set (node_setup)

set (node_setup_cpp

	nodes/setup/src/main.cpp	
	nodes/setup/src/controller.cpp
)

set (node_setup_h

	nodes/setup/src/controller.h
	nodes/setup/src/tasks/setup_defs.h

)

set (node_setup_info
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
)

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

set (node_setup_res
)
	
if(WIN32)

	set (node_setup_service
		nodes/setup/templates/setup_create_service.bat.in
		nodes/setup/templates/setup_delete_service.bat.in
		nodes/setup/templates/setup_restart_service.bat.in
		nodes/setup/templates/setup.windows.cgf.in
	)
 
else()

	set (node_setup_service
		nodes/setup/templates/setup.service.in
		nodes/setup/templates/setup.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_setup_tasks})
source_group("resources" FILES ${node_setup_res})
source_group("service" FILES ${node_setup_service})
source_group("info" FILES ${node_setup_info})


# define the main program
set (node_setup
  ${node_setup_cpp}
  ${node_setup_h}
  ${node_setup_tasks}
  ${node_setup_res}
  ${node_setup_service}
  ${node_setup_info}
)

