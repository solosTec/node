# top level files
set (node_ipt_gateway)

set (node_ipt_gateway_cpp

	nodes/ipt/gateway/src/main.cpp	
	nodes/ipt/gateway/src/controller.cpp
)

set (node_ipt_gateway_h

	nodes/ipt/gateway/src/controller.h

)

set (node_ipt_gateway_info
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

set (node_ipt_gateway_tasks
	nodes/ipt/gateway/src/tasks/network.h
	nodes/ipt/gateway/src/tasks/network.cpp
)

set (node_ipt_gateway_res
)
	
if(WIN32)

	set (node_ipt_gateway_service
		nodes/ipt/gateway/templates/gateway_create_service.bat.in
		nodes/ipt/gateway/templates/gateway_delete_service.bat.in
		nodes/ipt/gateway/templates/gateway_restart_service.bat.in
		nodes/ipt/gateway/templates/gateway.windows.cgf.in
	)
 
else()

	set (node_ipt_gateway_service
		nodes/ipt/gateway/templates/gateway.service.in
		nodes/ipt/gateway/templates/gateway.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_ipt_gateway_tasks})
source_group("resources" FILES ${node_ipt_gateway_res})
source_group("service" FILES ${node_ipt_gateway_service})
source_group("info" FILES ${node_ipt_gateway_info})


# define the main program
set (node_ipt_gateway
  ${node_ipt_gateway_cpp}
  ${node_ipt_gateway_h}
  ${node_ipt_gateway_tasks}
  ${node_ipt_gateway_res}
  ${node_ipt_gateway_service}
  ${node_ipt_gateway_info}
)
