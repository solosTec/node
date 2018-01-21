# top level files
set (node_ipt_store)

set (node_ipt_store_cpp

	nodes/ipt/store/src/main.cpp
	nodes/ipt/store/src/controller.cpp
)

set (node_ipt_store_h

	nodes/ipt/store/src/controller.h

)

set (node_ipt_store_info
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

set (node_ipt_store_tasks
	nodes/ipt/store/src/tasks/network.h
	nodes/ipt/store/src/tasks/network.cpp
	nodes/ipt/store/src/tasks/storage_db.h
	nodes/ipt/store/src/tasks/storage_db.cpp
)

set (node_ipt_store_res
)
	
if(WIN32)

	set (node_ipt_store_service
		nodes/ipt/store/templates/store_create_service.bat.in
		nodes/ipt/store/templates/store_delete_service.bat.in
		nodes/ipt/store/templates/store_restart_service.bat.in
		nodes/ipt/store/templates/store.windows.cgf.in
	)
 
else()

	set (node_ipt_store_service
		nodes/ipt/store/templates/store.service.in
		nodes/ipt/store/templates/store.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_ipt_store_tasks})
source_group("resources" FILES ${node_ipt_store_res})
source_group("service" FILES ${node_ipt_store_service})
source_group("info" FILES ${node_ipt_store_info})


# define the main program
set (node_ipt_store
  ${node_ipt_store_cpp}
  ${node_ipt_store_h}
  ${node_ipt_store_tasks}
  ${node_ipt_store_res}
  ${node_ipt_store_service}
  ${node_ipt_store_info}
)

