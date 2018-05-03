# top level files
set (node_ipt_master)

set (node_ipt_master_cpp

	nodes/ipt/master/src/main.cpp	
	nodes/ipt/master/src/controller.cpp
	nodes/ipt/master/src/connection.cpp
	nodes/ipt/master/src/server.cpp
	nodes/ipt/master/src/session.cpp
)

set (node_ipt_master_h

	nodes/ipt/master/src/controller.h
	nodes/ipt/master/src/connection.h
	nodes/ipt/master/src/server.h
	nodes/ipt/master/src/session.h

)

set (node_ipt_master_info
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

set (node_ipt_master_tasks
	nodes/ipt/master/src/tasks/cluster.h
	nodes/ipt/master/src/tasks/cluster.cpp
	nodes/ipt/master/src/tasks/open_connection.h
	nodes/ipt/master/src/tasks/open_connection.cpp
	nodes/ipt/master/src/tasks/close_connection.h
	nodes/ipt/master/src/tasks/close_connection.cpp
	nodes/ipt/master/src/tasks/gatekeeper.h
	nodes/ipt/master/src/tasks/gatekeeper.cpp
)

set (node_ipt_master_res
)
	
if(WIN32)

	set (node_ipt_master_service
		nodes/ipt/master/templates/ipt_create_service.cmd.in
		nodes/ipt/master/templates/ipt_delete_service.cmd.in
		nodes/ipt/master/templates/ipt_restart_service.cmd.in
		nodes/ipt/master/templates/ipt.windows.cgf.in
	)
 
else()

	set (node_ipt_master_service
		nodes/ipt/master/templates/ipt.service.in
		nodes/ipt/master/templates/ipt.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_ipt_master_tasks})
source_group("resources" FILES ${node_ipt_master_res})
source_group("service" FILES ${node_ipt_master_service})
source_group("info" FILES ${node_ipt_master_info})


# define the main program
set (node_ipt_master
  ${node_ipt_master_cpp}
  ${node_ipt_master_h}
  ${node_ipt_master_tasks}
  ${node_ipt_master_res}
  ${node_ipt_master_service}
  ${node_ipt_master_info}
)

