# top level files
set (node_ipt_collector)

set (node_ipt_collector_cpp

	src/main.cpp	
	src/controller.cpp
)

set (node_ipt_collector_h

	src/controller.h

)

set (node_ipt_collector_shared
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h

)

if (UNIX)
	list(APPEND node_ipt_collector_shared src/main/include/smf/shared/write_pid.h)
endif(UNIX)


set (node_ipt_collector_tasks
	src/tasks/network.h
	src/tasks/network.cpp
)
	
if(WIN32)

	set (node_ipt_collector_service
		templates/collector_create_service.cmd.in
		templates/collector_delete_service.cmd.in
		templates/collector_restart_service.cmd.in
		templates/collector.windows.cgf.in
	)
 
else()

	set (node_ipt_collector_service
		templates/collector.service.in
		templates/collector.linux.cgf.in
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

