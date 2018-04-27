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
	nodes/ipt/store/src/tasks/storage_xml.h
	nodes/ipt/store/src/tasks/storage_xml.cpp
	nodes/ipt/store/src/tasks/storage_json.h
	nodes/ipt/store/src/tasks/storage_json.cpp
	nodes/ipt/store/src/tasks/storage_abl.h
	nodes/ipt/store/src/tasks/storage_abl.cpp
	nodes/ipt/store/src/tasks/storage_binary.h
	nodes/ipt/store/src/tasks/storage_binary.cpp
	nodes/ipt/store/src/tasks/storage_log.h
	nodes/ipt/store/src/tasks/storage_log.cpp

	nodes/ipt/store/src/tasks/write_json.h
	nodes/ipt/store/src/tasks/write_json.cpp
	nodes/ipt/store/src/tasks/write_abl.h
	nodes/ipt/store/src/tasks/write_abl.cpp
	nodes/ipt/store/src/tasks/write_log.h
	nodes/ipt/store/src/tasks/write_log.cpp

)
set (node_ipt_store_processors
	nodes/ipt/store/src/processors/xml_processor.h
	nodes/ipt/store/src/processors/xml_processor.cpp
	nodes/ipt/store/src/processors/db_processor.h
	nodes/ipt/store/src/processors/db_processor.cpp
)

set (node_ipt_store_exporter

	src/main/include/smf/sml/exporter/xml_exporter.h
	lib/sml/exporter/src/xml_exporter.cpp
	src/main/include/smf/sml/exporter/db_exporter.h
	lib/sml/exporter/src/db_exporter.cpp

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
source_group("exporter" FILES ${node_ipt_store_exporter})
source_group("service" FILES ${node_ipt_store_service})
source_group("info" FILES ${node_ipt_store_info})
source_group("processors" FILES ${node_ipt_store_processors})


# define the main program
set (node_ipt_store
  ${node_ipt_store_cpp}
  ${node_ipt_store_h}
  ${node_ipt_store_tasks}
  ${node_ipt_store_exporter}
  ${node_ipt_store_service}
  ${node_ipt_store_info}
  ${node_ipt_store_processors}
)

if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	set (node_ipt_store_xml
		${PUGIXML_INCLUDE_DIR}/pugixml.hpp
		${PUGIXML_INCLUDE_DIR}/pugixml.cpp
	)
	list(APPEND node_ipt_store ${node_ipt_store_xml})
	source_group("XML" FILES ${node_ipt_store_xml})

endif()
