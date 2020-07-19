# top level files
set (node_lora)

set (node_lora_cpp

	src/main.cpp	
	src/controller.cpp
	src/processor.cpp
	src/dispatcher.cpp
	src/sync_db.cpp
)

set (node_lora_h

	src/controller.h
	src/processor.h
	src/dispatcher.h
	src/sync_db.h

)
set (node_lora_shared
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h

)

if (UNIX)
	list(APPEND node_lora_shared ${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/write_pid.h)
	list(APPEND node_lora_shared ${CMAKE_SOURCE_DIR}/nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (node_lora_tasks
	src/tasks/cluster.h
	src/tasks/cluster.cpp
)

set (node_lora_assets
	src/assets/index.html
)
	
set (node_lora_examples
	src/examples/example.xml
	src/examples/water.xml
)

set (node_lora_schemes

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/db_schemes.h
	${CMAKE_SOURCE_DIR}/nodes/shared/db/db_schemes.cpp
)

if(WIN32)

	set (node_lora_service
		templates/lora_create_service.cmd.in
		templates/lora_delete_service.cmd.in
		templates/lora_restart_service.cmd.in
		templates/lora.windows.cgf.in
	)
 
else()

	set (node_lora_service
		templates/lora.service.in
		templates/lora.linux.cgf.in
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
