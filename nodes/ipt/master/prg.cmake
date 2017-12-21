# top level files
set (node_ipt_master)

set (node_ipt_master_cpp

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/ipt/master/src/main.cpp	
	nodes/ipt/master/src/controller.cpp
)

set (node_ipt_master_h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/ipt/master/src/controller.h

)

set (node_ipt_master_tasks
)

set (node_ipt_master_res
)
	
# if(WIN32)
# 
# 	set (ipt_masterboard_service
# 		src/main/templates/create_ipt_master_srv.bat.in
# 		src/main/templates/delete_ipt_master_srv.bat.in
# 		src/main/templates/restart_ipt_master_srv.bat.in
# 		src/main/templates/ipt_master.windows.cgf.in
# 	)
# 
# else()
# 
# 	set (ipt_masterboard_service
# 		src/main/templates/ipt_master.service.in
# 		src/main/templates/ipt_master.windows.cgf.in
# 	)
# 
# endif()

source_group("tasks" FILES ${node_ipt_master_tasks})
source_group("resources" FILES ${node_ipt_master_res})
# source_group("service" FILES ${node_ipt_master_service})


# define the main program
set (node_ipt_master
  ${node_ipt_master_cpp}
  ${node_ipt_master_h}
  ${node_ipt_master_tasks}
  ${node_ipt_master_res}
#   ${node_ipt_master_service}
)

