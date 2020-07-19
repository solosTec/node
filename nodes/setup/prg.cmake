# top level files
set (node_setup)

set (node_setup_cpp

	src/main.cpp	
	src/controller.cpp 
	src/tasks/tables.cpp
)

set (node_setup_h

	src/controller.h
	src/tasks/setup_defs.h
	src/tasks/tables.h
)

set (node_setup_shared
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h
	${CMAKE_SOURCE_DIR}/nodes/show_fs_drives.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h
)


if (UNIX)
	list(APPEND node_setup_shared ${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/write_pid.h)
	list(APPEND node_setup_shared ${CMAKE_SOURCE_DIR}/nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (node_setup_tasks
	src/tasks/cluster.h
	src/tasks/cluster.cpp
	src/tasks/storage_db.h
	src/tasks/storage_db.cpp
	src/tasks/storage_json.h
	src/tasks/storage_json.cpp
	src/tasks/storage_xml.h
	src/tasks/storage_xml.cpp
	src/tasks/sync.h
	src/tasks/sync.cpp
)

set (node_setup_schemes

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/db_schemes.h
	${CMAKE_SOURCE_DIR}/nodes/shared/db/db_schemes.cpp
)

if(WIN32)

	set (node_setup_service
		templates/setup_create_service.cmd.in
		templates/setup_delete_service.cmd.in
		templates/setup_restart_service.cmd.in
		templates/setup.windows.cgf.in
	)
 
 	set (node_setup_res
		${CMAKE_BINARY_DIR}/setup.rc 
		${CMAKE_SOURCE_DIR}/src/main/resources/logo.ico
		templates/setup.exe.manifest
	)

else()

	set (node_setup_service
		templates/setup.service.in
		templates/setup.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_setup_tasks})
source_group("service" FILES ${node_setup_service})
source_group("shared" FILES ${node_setup_shared})
source_group("schemes" FILES ${node_setup_schemes})


# define the main program
set (node_setup
  ${node_setup_cpp}
  ${node_setup_h}
  ${node_setup_tasks}
  ${node_setup_service}
  ${node_setup_shared}
  ${node_setup_schemes}
)

if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	set (node_setup_xml
		${PUGIXML_INCLUDE_DIR}/pugixml.hpp
		${PUGIXML_INCLUDE_DIR}/pugixml.cpp
	)
	list(APPEND node_setup ${node_setup_xml})
	source_group("XML" FILES ${node_setup_xml})

endif()

if(WIN32)
	source_group("resources" FILES ${node_setup_res})
	list(APPEND node_setup ${node_setup_res})
endif()
