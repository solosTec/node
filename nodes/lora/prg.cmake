# top level files
set (node_lora)

set (node_lora_cpp

	nodes/lora/src/main.cpp	
	nodes/lora/src/controller.cpp
	nodes/lora/src/processor.cpp
)

set (node_lora_h

	nodes/lora/src/controller.h
	nodes/lora/src/processor.h

)
set (node_lora_info
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/write_pid.h

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/write_pid.cpp
)

set (node_lora_tasks
	nodes/lora/src/tasks/cluster.h
	nodes/lora/src/tasks/cluster.cpp
)

set (node_lora_assets
	nodes/lora/src/assets/index.html
)
	
set (node_lora_examples
	nodes/lora/src/examples/example.xml
)

if(WIN32)

	set (node_lora_service
		nodes/lora/templates/lora_create_service.cmd.in
		nodes/lora/templates/lora_delete_service.cmd.in
		nodes/lora/templates/lora_restart_service.cmd.in
		nodes/lora/templates/lora.windows.cgf.in
	)
 
else()

	set (node_lora_service
		nodes/lora/templates/lora.service.in
		nodes/lora/templates/lora.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_lora_tasks})
source_group("service" FILES ${node_lora_service})
source_group("resources" FILES ${node_lora_info})
source_group("assets" FILES ${node_lora_assets})
source_group("examples" FILES ${node_lora_examples})


# define the main program
set (node_lora
  ${node_lora_cpp}
  ${node_lora_h}
  ${node_lora_tasks}
  ${node_lora_res}
  ${node_lora_service}
  ${node_lora_info}
  ${node_lora_assets}
  ${node_lora_examples}
)


if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	set (node_lora_xml
		${PUGIXML_INCLUDE_DIR}/pugixml.hpp
		${PUGIXML_INCLUDE_DIR}/pugixml.cpp
	)
	list(APPEND node_lora ${node_lora_xml})
	source_group("XML" FILES ${node_lora_xml})

endif()
