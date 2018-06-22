# top level files
set (node_iec)

set (node_iec_cpp

	nodes/iec/src/main.cpp	
	nodes/iec/src/controller.cpp
	#nodes/iec/src/processor.cpp
)

set (node_iec_h

	nodes/iec/src/controller.h
	#nodes/iec/src/processor.h

)
set (node_iec_info
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

set (node_iec_tasks
	nodes/iec/src/tasks/cluster.h
	nodes/iec/src/tasks/cluster.cpp
)

set (node_iec_res
)
	
if(WIN32)

	set (node_iec_service
		nodes/iec/templates/iec_create_service.cmd.in
		nodes/iec/templates/iec_delete_service.cmd.in
		nodes/iec/templates/iec_restart_service.cmd.in
		nodes/iec/templates/iec.windows.cgf.in
	)
 
else()

	set (node_iec_service
		nodes/iec/templates/iec.service.in
		nodes/iec/templates/iec.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_iec_tasks})
source_group("resources" FILES ${node_iec_res})
source_group("service" FILES ${node_iec_service})
source_group("resources" FILES ${node_iec_info})


# define the main program
set (node_iec
  ${node_iec_cpp}
  ${node_iec_h}
  ${node_iec_tasks}
  ${node_iec_res}
  ${node_iec_service}
  ${node_iec_info}
)
