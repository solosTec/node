# top level files
set (node_ipt_store)

set (node_ipt_store_cpp

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/ipt/store/src/main.cpp
	nodes/ipt/store/src/controller.cpp
)

set (node_ipt_store_h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/ipt/store/src/controller.h

)

set (node_ipt_store_tasks
)

set (node_ipt_store_res
)
	
# if(WIN32)
# 
# 	set (ipt_storeboard_service
# 		src/main/templates/create_ipt_store_srv.bat.in
# 		src/main/templates/delete_ipt_store_srv.bat.in
# 		src/main/templates/restart_ipt_store_srv.bat.in
# 		src/main/templates/ipt_store.windows.cgf.in
# 	)
# 
# else()
# 
# 	set (ipt_storeboard_service
# 		src/main/templates/ipt_store.service.in
# 		src/main/templates/ipt_store.windows.cgf.in
# 	)
# 
# endif()

source_group("tasks" FILES ${node_ipt_store_tasks})
source_group("resources" FILES ${node_ipt_store_res})
# source_group("service" FILES ${node_ipt_store_service})


# define the main program
set (node_ipt_store
  ${node_ipt_store_cpp}
  ${node_ipt_store_h}
  ${node_ipt_store_tasks}
  ${node_ipt_store_res}
#   ${node_ipt_store_service}
)

