# top level files
set (node_lora)

set (node_lora_cpp

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/lora/src/main.cpp	
	nodes/lora/src/controller.cpp
)

set (node_lora_h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/lora/src/controller.h

)

set (node_lora_tasks
)

set (node_lora_res
)
	
# if(WIN32)
# 
# 	set (loraboard_service
# 		src/main/templates/create_lora_srv.bat.in
# 		src/main/templates/delete_lora_srv.bat.in
# 		src/main/templates/restart_lora_srv.bat.in
# 		src/main/templates/lora.windows.cgf.in
# 	)
# 
# else()
# 
# 	set (loraboard_service
# 		src/main/templates/lora.service.in
# 		src/main/templates/lora.windows.cgf.in
# 	)
# 
# endif()

source_group("tasks" FILES ${node_lora_tasks})
source_group("resources" FILES ${node_lora_res})
# source_group("service" FILES ${node_lora_service})


# define the main program
set (node_lora
  ${node_lora_cpp}
  ${node_lora_h}
  ${node_lora_tasks}
  ${node_lora_res}
#   ${node_lora_service}
)

