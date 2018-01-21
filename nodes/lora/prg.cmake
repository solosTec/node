# top level files
set (node_lora)

set (node_lora_cpp

	nodes/lora/src/main.cpp	
	nodes/lora/src/controller.cpp
)

set (node_lora_h

	nodes/lora/src/controller.h

)
set (node_lora_info
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

set (node_lora_tasks
	nodes/lora/src/tasks/cluster.h
	nodes/lora/src/tasks/cluster.cpp
)

set (node_lora_res
)
	
if(WIN32)

	set (node_lora_service
		nodes/lora/templates/lora_create_service.bat.in
		nodes/lora/templates/lora_delete_service.bat.in
		nodes/lora/templates/lora_restart_service.bat.in
		nodes/lora/templates/lora.windows.cgf.in
	)
 
else()

	set (node_lora_service
		nodes/lora/templates/lora.service.in
		nodes/lora/templates/lora.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_lora_tasks})
source_group("resources" FILES ${node_lora_res})
source_group("service" FILES ${node_lora_service})
source_group("resources" FILES ${node_lora_info})


# define the main program
set (node_lora
  ${node_lora_cpp}
  ${node_lora_h}
  ${node_lora_tasks}
  ${node_lora_res}
  ${node_lora_service}
  ${node_lora_info}
)

