# top level files
set (node_ipt_gateway)

set (node_ipt_gateway_cpp

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/ipt/gateway/src/main.cpp	
	nodes/ipt/gateway/src/controller.cpp
)

set (node_ipt_gateway_h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/ipt/gateway/src/controller.h

)

set (node_ipt_gateway_tasks
)

set (node_ipt_gateway_res
)
	
# if(WIN32)
# 
# 	set (ipt_gatewayboard_service
# 		src/main/templates/create_ipt_gateway_srv.bat.in
# 		src/main/templates/delete_ipt_gateway_srv.bat.in
# 		src/main/templates/restart_ipt_gateway_srv.bat.in
# 		src/main/templates/ipt_gateway.windows.cgf.in
# 	)
# 
# else()
# 
# 	set (ipt_gatewayboard_service
# 		src/main/templates/ipt_gateway.service.in
# 		src/main/templates/ipt_gateway.windows.cgf.in
# 	)
# 
# endif()

source_group("tasks" FILES ${node_ipt_gateway_tasks})
source_group("resources" FILES ${node_ipt_gateway_res})
# source_group("service" FILES ${node_ipt_gateway_service})


# define the main program
set (node_ipt_gateway
  ${node_ipt_gateway_cpp}
  ${node_ipt_gateway_h}
  ${node_ipt_gateway_tasks}
  ${node_ipt_gateway_res}
#   ${node_ipt_gateway_service}
)

