# top level files
set (node_iec_62056)

set (node_iec_62056_cpp

	nodes/iec-62056/src/main.cpp	
	nodes/iec-62056/src/controller.cpp
)

set (node_iec_62056_h

	nodes/iec-62056/src/controller.h

)

set (node_iec_62056_info
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

set (node_iec_62056_tasks
	nodes/iec-62056/src/tasks/cluster.h
	nodes/iec-62056/src/tasks/cluster.cpp
)

	
if(WIN32)

	set (node_iec_62056_service
		nodes/iec-62056/templates/iec_62056_create_service.cmd.in
		nodes/iec-62056/templates/iec_62056_delete_service.cmd.in
		nodes/iec-62056/templates/iec_62056_restart_service.cmd.in
		nodes/iec-62056/templates/iec_62056.windows.cgf.in
	)
 
else()

	set (node_iec_62056_service
		nodes/iec-62056/templates/iec_62056.service.in
		nodes/iec-62056/templates/iec_62056.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_iec_62056_tasks})
source_group("resources" FILES ${node_iec_62056_res})
source_group("service" FILES ${node_iec_62056_service})
source_group("resources" FILES ${node_iec_62056_info})


# define the main program
set (node_iec_62056
  ${node_iec_62056_cpp}
  ${node_iec_62056_h}
  ${node_iec_62056_tasks}
  ${node_iec_62056_service}
  ${node_iec_62056_info}
)


