# top level files
set (node_setup)

set (node_setup_cpp

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/setup/src/main.cpp	
	nodes/setup/src/controller.cpp
)

set (node_setup_h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/setup/src/controller.h

)

set (node_setup_tasks
)

set (node_setup_res
)
	
# if(WIN32)
# 
# 	set (setupboard_service
# 		src/main/templates/create_setup_srv.bat.in
# 		src/main/templates/delete_setup_srv.bat.in
# 		src/main/templates/restart_setup_srv.bat.in
# 		src/main/templates/setup.windows.cgf.in
# 	)
# 
# else()
# 
# 	set (setupboard_service
# 		src/main/templates/setup.service.in
# 		src/main/templates/setup.windows.cgf.in
# 	)
# 
# endif()

source_group("tasks" FILES ${node_setup_tasks})
source_group("resources" FILES ${node_setup_res})
# source_group("service" FILES ${node_setup_service})


# define the main program
set (node_setup
  ${node_setup_cpp}
  ${node_setup_h}
  ${node_setup_tasks}
  ${node_setup_res}
#   ${node_setup_service}
)

