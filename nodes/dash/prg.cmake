# top level files
set (node_dash)

set (node_dash_cpp

	nodes/dash/src/main.cpp	
	nodes/dash/src/controller.cpp
)

set (node_dash_h

	nodes/dash/src/controller.h

)

set (node_dash_tasks
	nodes/dash/src/tasks/cluster.h
	nodes/dash/src/tasks/cluster.cpp
)

set (node_dash_res
	nodes/dash/htdocs/index.html
	nodes/dash/htdocs/config.device.html
	nodes/dash/htdocs/config.gateway.html
	nodes/dash/htdocs/config.meter.html
	nodes/dash/htdocs/status.session.html
	nodes/dash/htdocs/status.system.html
	nodes/dash/htdocs/status.targets.html
	nodes/dash/htdocs/status.connections.html
	nodes/dash/htdocs/monitor.msg.html
)

set (node_dash_info
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

	
if(WIN32)

	set (node_dash_service
		nodes/dash/templates/dash_create_service.bat.in
		nodes/dash/templates/dash_delete_service.bat.in
		nodes/dash/templates/dash_restart_service.bat.in
		nodes/dash/templates/dash.windows.cgf.in
	)
 
else()

	set (node_dash_service
		nodes/dash/templates/dash.service.in
		nodes/dash/templates/dash.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_dash_tasks})
source_group("resources" FILES ${node_dash_res})
source_group("service" FILES ${node_dash_service})
source_group("info" FILES ${node_dash_info})


# define the main program
set (node_dash
  ${node_dash_cpp}
  ${node_dash_h}
  ${node_dash_tasks}
  ${node_dash_res}
  ${node_dash_service}
  ${node_dash_info}
)

