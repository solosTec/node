# top level files
set (node_ipt_collector)

set (node_ipt_collector_cpp

	nodes/ipt/collector/src/main.cpp	
	nodes/ipt/collector/src/controller.cpp
)

set (node_ipt_collector_h

	nodes/ipt/collector/src/controller.h

)

set (node_ipt_collector_info
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

set (node_ipt_collector_tasks
	nodes/ipt/collector/src/tasks/network.h
	nodes/ipt/collector/src/tasks/network.cpp
)

set (node_ipt_collector_res
)
	
if(WIN32)

	set (node_ipt_collector_service
		nodes/ipt/collector/templates/collector_create_service.bat.in
		nodes/ipt/collector/templates/collector_delete_service.bat.in
		nodes/ipt/collector/templates/collector_restart_service.bat.in
		nodes/ipt/collector/templates/collector.windows.cgf.in
	)
 
else()

	set (node_ipt_collector_service
		nodes/ipt/collector/templates/collector.service.in
		nodes/ipt/collector/templates/collector.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_ipt_collector_tasks})
source_group("resources" FILES ${node_ipt_collector_res})
source_group("service" FILES ${node_ipt_collector_service})
source_group("info" FILES ${node_ipt_collector_info})


# define the main program
set (node_ipt_collector
  ${node_ipt_collector_cpp}
  ${node_ipt_collector_h}
  ${node_ipt_collector_tasks}
  ${node_ipt_collector_res}
  ${node_ipt_collector_service}
  ${node_ipt_collector_info}
)

