# top level files
set (node_master)

set (node_master_cpp

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/master/src/main.cpp	
	nodes/master/src/controller.cpp
	nodes/master/src/server.cpp
	nodes/master/src/connection.cpp
)

set (node_master_h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/master/src/controller.h
	nodes/master/src/server.h
	nodes/master/src/connection.h
)

set (node_master_tasks
)

set (node_master_res
)
	
# if(WIN32)
# 
# 	set (masterboard_service
# 		src/main/templates/create_master_srv.bat.in
# 		src/main/templates/delete_master_srv.bat.in
# 		src/main/templates/restart_master_srv.bat.in
# 		src/main/templates/master.windows.cgf.in
# 	)
# 
# else()
# 
# 	set (masterboard_service
# 		src/main/templates/master.service.in
# 		src/main/templates/master.windows.cgf.in
# 	)
# 
# endif()

source_group("tasks" FILES ${node_master_tasks})
source_group("resources" FILES ${node_master_res})
# source_group("service" FILES ${node_master_service})


# define the main program
set (node_master
  ${node_master_cpp}
  ${node_master_h}
  ${node_master_tasks}
  ${node_master_res}
#   ${node_master_service}
)

