# top level files
set (node_ipt_store)

set (node_ipt_store_cpp

	src/main.cpp
	src/controller.cpp
)

set (node_ipt_store_h

	src/controller.h
	src/message_ids.h

)

set (node_ipt_store_schemes

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/db_meta.h
	${CMAKE_SOURCE_DIR}/nodes/shared/db/db_meta.cpp
)

set (node_ipt_store_shared
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h
	${CMAKE_SOURCE_DIR}/nodes/show_fs_drives.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h

)

if (UNIX)
	list(APPEND node_ipt_store_shared src/main/include/smf/shared/write_pid.h)
endif(UNIX)

set (node_ipt_store_tasks
	src/tasks/network.h
	src/tasks/network.cpp
	src/tasks/sml_to_db_consumer.h
	src/tasks/sml_to_db_consumer.cpp
	src/tasks/sml_to_xml_consumer.h
	src/tasks/sml_to_xml_consumer.cpp
	src/tasks/sml_to_json_consumer.h
	src/tasks/sml_to_json_consumer.cpp
	src/tasks/sml_to_abl_consumer.h
	src/tasks/sml_to_abl_consumer.cpp
	src/tasks/sml_to_log_consumer.h
	src/tasks/sml_to_log_consumer.cpp
	src/tasks/sml_to_csv_consumer.h
	src/tasks/sml_to_csv_consumer.cpp
	src/tasks/sml_to_influxdb_consumer.h
	src/tasks/sml_to_influxdb_consumer.cpp
	src/tasks/binary_consumer.h
	src/tasks/binary_consumer.cpp
	src/tasks/iec_to_db_consumer.h
	src/tasks/iec_to_db_consumer.cpp
)
set (node_ipt_store_processors
	src/processors/sml_processor.h
	src/processors/sml_processor.cpp
	src/processors/iec_processor.h
	src/processors/iec_processor.cpp
)

set (node_ipt_store_exporter

	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/exporter/xml_sml_exporter.h
	${CMAKE_SOURCE_DIR}/lib/sml/exporter/src/xml_sml_exporter.cpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/exporter/db_sml_exporter.h
	${CMAKE_SOURCE_DIR}/lib/sml/exporter/src/db_sml_exporter.cpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/exporter/csv_sml_exporter.h
	${CMAKE_SOURCE_DIR}/lib/sml/exporter/src/csv_sml_exporter.cpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/exporter/abl_sml_exporter.h
	${CMAKE_SOURCE_DIR}/lib/sml/exporter/src/abl_sml_exporter.cpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/exporter/influxdb_sml_exporter.h
	${CMAKE_SOURCE_DIR}/lib/sml/exporter/src/influxdb_sml_exporter.cpp
# maybe an IEC library should be used
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/exporter/db_iec_exporter.h
	${CMAKE_SOURCE_DIR}/lib/sml/exporter/src/db_iec_exporter.cpp
)
	
if(WIN32)

	set (node_ipt_store_service
		templates/store_create_service.cmd.in
		templates/store_delete_service.cmd.in
		templates/store_restart_service.cmd.in
		templates/store.windows.cgf.in
	)
 
 	set (node_ipt_store_manifest
		templates/store.exe.manifest
	)

 	set (node_ipt_store_res
		${CMAKE_BINARY_DIR}/store.rc 
		${CMAKE_SOURCE_DIR}/src/main/resources/logo.ico
	)

else()

	set (node_ipt_store_service
		templates/store.service.in
		templates/store.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_ipt_store_tasks})
source_group("exporter" FILES ${node_ipt_store_exporter})
source_group("service" FILES ${node_ipt_store_service})
source_group("shared" FILES ${node_ipt_store_shared})
source_group("processors" FILES ${node_ipt_store_processors})
source_group("schemes" FILES ${node_ipt_store_schemes})


# define the main program
set (node_ipt_store
  ${node_ipt_store_cpp}
  ${node_ipt_store_h}
  ${node_ipt_store_tasks}
  ${node_ipt_store_exporter}
  ${node_ipt_store_service}
  ${node_ipt_store_shared}
  ${node_ipt_store_processors}
  ${node_ipt_store_schemes}
)

if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	set (node_ipt_store_xml
		${PUGIXML_INCLUDE_DIR}/pugixml.hpp
		${PUGIXML_INCLUDE_DIR}/pugixml.cpp
	)
	list(APPEND node_ipt_store ${node_ipt_store_xml})
	source_group("XML" FILES ${node_ipt_store_xml})

endif()

if(WIN32)
	source_group("manifest" FILES ${node_ipt_store_manifest})
	list(APPEND node_ipt_store ${node_ipt_store_manifest})
	source_group("resources" FILES ${node_ipt_store_res})
	list(APPEND node_ipt_store ${node_ipt_store_res})
endif()
