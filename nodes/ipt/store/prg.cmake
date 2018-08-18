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
	nodes/write_pid.h

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/write_pid.cpp
)

set (node_ipt_store_tasks
	nodes/ipt/store/src/tasks/network.h
	nodes/ipt/store/src/tasks/network.cpp
	nodes/ipt/store/src/tasks/sml_to_db_consumer.h
	nodes/ipt/store/src/tasks/sml_to_db_consumer.cpp
	nodes/ipt/store/src/tasks/sml_to_xml_consumer.h
	nodes/ipt/store/src/tasks/sml_to_xml_consumer.cpp
	nodes/ipt/store/src/tasks/sml_to_json_consumer.h
	nodes/ipt/store/src/tasks/sml_to_json_consumer.cpp
	nodes/ipt/store/src/tasks/sml_to_abl_consumer.h
	nodes/ipt/store/src/tasks/sml_to_abl_consumer.cpp
	nodes/ipt/store/src/tasks/sml_to_log_consumer.h
	nodes/ipt/store/src/tasks/sml_to_log_consumer.cpp
	nodes/ipt/store/src/tasks/sml_to_csv_consumer.h
	nodes/ipt/store/src/tasks/sml_to_csv_consumer.cpp
	nodes/ipt/store/src/tasks/binary_consumer.h
	nodes/ipt/store/src/tasks/binary_consumer.cpp
	nodes/ipt/store/src/tasks/iec_to_db_consumer.h
	nodes/ipt/store/src/tasks/iec_to_db_consumer.cpp
)
set (node_ipt_store_processors
	nodes/ipt/store/src/processors/sml_processor.h
	nodes/ipt/store/src/processors/sml_processor.cpp
	nodes/ipt/store/src/processors/iec_processor.h
	nodes/ipt/store/src/processors/iec_processor.cpp
)

set (node_ipt_store_exporter

	src/main/include/smf/sml/exporter/xml_exporter.h
	lib/sml/exporter/src/xml_exporter.cpp
	src/main/include/smf/sml/exporter/db_exporter.h
	lib/sml/exporter/src/db_exporter.cpp
	src/main/include/smf/sml/exporter/csv_exporter.h
	lib/sml/exporter/src/csv_exporter.cpp

)
	
if(WIN32)

	set (node_ipt_store_service
		nodes/ipt/store/templates/store_create_service.cmd.in
		nodes/ipt/store/templates/store_delete_service.cmd.in
		nodes/ipt/store/templates/store_restart_service.cmd.in
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
