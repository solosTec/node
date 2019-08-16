# top level files
set (node_lora)

set (node_lora_cpp

	nodes/lora/src/main.cpp	
	nodes/lora/src/controller.cpp
	nodes/lora/src/processor.cpp
	nodes/lora/src/dispatcher.cpp
	nodes/lora/src/sync_db.cpp
)

set (node_lora_h

	nodes/lora/src/controller.h
	nodes/lora/src/processor.h
	nodes/lora/src/dispatcher.h
	nodes/lora/src/sync_db.h

)
set (node_lora_shared
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp

	src/main/include/smf/shared/ctl.h
	nodes/shared/sys/ctl.cpp

)

if (UNIX)
	list(APPEND node_lora_shared src/main/include/smf/shared/write_pid.h)
	list(APPEND node_lora_shared nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (node_lora_tasks
	nodes/lora/src/tasks/cluster.h
	nodes/lora/src/tasks/cluster.cpp
)

set (node_lora_assets
	nodes/lora/src/assets/index.html
)
	
set (node_lora_examples
	nodes/lora/src/examples/example.xml
	nodes/lora/src/examples/water.xml
)

set (node_lora_schemes

	src/main/include/smf/shared/db_schemes.h
	nodes/shared/db/db_schemes.cpp
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
source_group("shared" FILES ${node_lora_shared})
source_group("assets" FILES ${node_lora_assets})
source_group("examples" FILES ${node_lora_examples})
source_group("schemes" FILES ${node_lora_schemes})


# define the main program
set (node_lora
  ${node_lora_cpp}
  ${node_lora_h}
  ${node_lora_tasks}
  ${node_lora_res}
  ${node_lora_service}
  ${node_lora_shared}
  ${node_lora_assets}
  ${node_lora_examples}
  ${node_lora_schemes}
)


if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	set (node_lora_xml
		${PUGIXML_INCLUDE_DIR}/pugixml.hpp
		${PUGIXML_INCLUDE_DIR}/pugixml.cpp
	)
	list(APPEND node_lora ${node_lora_xml})
	source_group("XML" FILES ${node_lora_xml})

endif()
