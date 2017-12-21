# top level files
set (node_dash)

set (node_dash_cpp

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/dash/src/main.cpp	
	nodes/dash/src/controller.cpp
)

set (node_dash_h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/dash/src/controller.h

)

set (node_dash_tasks
)

set (node_dash_res
)
	
# if(WIN32)
# 
# 	set (dashboard_service
# 		src/main/templates/create_dash_srv.bat.in
# 		src/main/templates/delete_dash_srv.bat.in
# 		src/main/templates/restart_dash_srv.bat.in
# 		src/main/templates/dash.windows.cgf.in
# 	)
# 
# else()
# 
# 	set (dashboard_service
# 		src/main/templates/dash.service.in
# 		src/main/templates/dash.windows.cgf.in
# 	)
# 
# endif()

source_group("tasks" FILES ${node_dash_tasks})
source_group("resources" FILES ${node_dash_res})
# source_group("service" FILES ${node_dash_service})


# define the main program
set (node_dash
  ${node_dash_cpp}
  ${node_dash_h}
  ${node_dash_tasks}
  ${node_dash_res}
#   ${node_dash_service}
)

