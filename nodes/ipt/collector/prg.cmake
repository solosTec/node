# top level files
set (node_ipt_collector)

set (node_ipt_collector_cpp

	nodes/ipt/collector/src/main.cpp	
	nodes/ipt/collector/src/controller.cpp
)

set (node_ipt_collector_h

	nodes/ipt/collector/src/controller.h

)

set (node_ipt_collector_shared
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h

	src/main/include/smf/shared/ctl.h

)

if (UNIX)
	list(APPEND node_ipt_collector_shared src/main/include/smf/shared/write_pid.h)
endif(UNIX)


set (node_ipt_collector_tasks
	nodes/ipt/collector/src/tasks/network.h
	nodes/ipt/collector/src/tasks/network.cpp
)
	
if(WIN32)

	set (node_ipt_collector_service
		nodes/ipt/collector/templates/collector_create_service.cmd.in
		nodes/ipt/collector/templates/collector_delete_service.cmd.in
		nodes/ipt/collector/templates/collector_restart_service.cmd.in
		nodes/ipt/collector/templates/collector.windows.cgf.in
	)
 
else()

	set (node_ipt_collector_service
		nodes/ipt/collector/templates/collector.service.in
		nodes/ipt/collector/templates/collector.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_ipt_collector_tasks})
source_group("service" FILES ${node_ipt_collector_service})
source_group("info" FILES ${node_ipt_collector_shared})


# define the main program
set (node_ipt_collector
  ${node_ipt_collector_cpp}
  ${node_ipt_collector_h}
  ${node_ipt_collector_tasks}
  ${node_ipt_collector_service}
  ${node_ipt_collector_shared}
)

