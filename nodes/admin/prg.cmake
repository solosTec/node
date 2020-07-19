# top level files
set (node_dash)
set (node_dashs)

set (node_dash_cpp

	dash/src/main.cpp	
	dash/src/controller.cpp
)

set (node_dash_h

	dash/src/controller.h

)

set (node_dash_tasks
	dash/src/tasks/cluster.h
	dash/src/tasks/cluster.cpp
	dash/src/tasks/system.h
	dash/src/tasks/system.cpp
)

set (node_admin_shared
	shared/src/sync_db.h
	shared/src/sync_db.cpp
	shared/src/dispatcher.h
	shared/src/dispatcher.cpp
	shared/src/forwarder.h
	shared/src/forwarder.cpp
	shared/src/form_data.h
	shared/src/form_data.cpp

	shared/src/tables.h
	shared/src/tables.cpp

	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h
#	${CMAKE_SOURCE_DIR}/nodes/shared/sys/ctl.cpp

)

set (node_schemes

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/db_schemes.h
	${CMAKE_SOURCE_DIR}/nodes/shared/db/db_schemes.cpp
)
	
if(WIN32)

	set (node_dash_service
		templates/dash_create_service.cmd.in
		templates/dash_delete_service.cmd.in
		templates/dash_restart_service.cmd.in
		templates/dash.windows.cgf.in
	)
 
 	set (node_dash_res
		${CMAKE_BINARY_DIR}/dash.rc 
		${CMAKE_SOURCE_DIR}/src/main/resources/logo.ico
		templates/dash.exe.manifest
	)

else()

	set (node_dash_service
		templates/dash.service.in
		templates/dash.linux.cgf.in
	)

endif()

source_group("dash/tasks" FILES ${node_dash_tasks})
source_group("dash/service" FILES ${node_dash_service})
source_group("admin" FILES ${node_admin_shared})
source_group("schemes" FILES ${node_schemes})


# define the main program
set (node_dash
  ${node_dash_cpp}
  ${node_dash_h}
  ${node_dash_tasks}
  ${node_dash_service}
  ${node_admin_shared}
  ${node_schemes}
)

if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	set (node_dash_xml
		${PUGIXML_INCLUDE_DIR}/pugixml.hpp
		${PUGIXML_INCLUDE_DIR}/pugixml.cpp
	)
	list(APPEND node_dash ${node_dash_xml})
	source_group("dash/XML" FILES ${node_dash_xml})

endif()

if(WIN32)
	source_group("dash/resources" FILES ${node_dash_res})
	list(APPEND node_dash ${node_dash_res})
endif()

# --------------------------------------------------------------------+
set (node_dashs_cpp

	dashs/src/main.cpp	
	dashs/src/controller.cpp
)

set (node_dashs_h

	dashs/src/controller.h

)

set (node_dashs_tasks
	dashs/src/tasks/cluster.h
	dashs/src/tasks/cluster.cpp
	dashs/src/tasks/system.h
	dashs/src/tasks/system.cpp
)

set (node_dashs_schemes

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/db_schemes.h
	${CMAKE_SOURCE_DIR}/nodes/shared/db/db_schemes.cpp
)
	
if(WIN32)

	set (node_dashs_service
		templates/dashs_create_service.cmd.in
		templates/dashs_delete_service.cmd.in
		templates/dashs_restart_service.cmd.in
		templates/dashs.windows.cgf.in
	)

 	set (node_dashs_res
		${CMAKE_BINARY_DIR}/dashs.rc 
		${CMAKE_SOURCE_DIR}/src/main/resources/logo.ico
		templates/dashs.exe.manifest
	)

else()

	set (node_dashs_service
		templates/dashs.service.in
		templates/dashs.linux.cgf.in
	)

endif()

source_group("dashs/tasks" FILES ${node_dashs_tasks})
source_group("dashs/service" FILES ${node_dashs_service})


# define the main program
set (node_dashs
  ${node_dashs_cpp}
  ${node_dashs_h}
  ${node_dashs_tasks}
  ${node_dashs_service}
  ${node_admin_shared}
  ${node_schemes}
)

if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	set (node_dashs_xml
		${PUGIXML_INCLUDE_DIR}/pugixml.hpp
		${PUGIXML_INCLUDE_DIR}/pugixml.cpp
	)
	list(APPEND node_dashs ${node_dashs_xml})
	source_group("dashs/XML" FILES ${node_dashs_xml})

endif()

if(WIN32)
	source_group("dashs/resources" FILES ${node_dashs_res})
	list(APPEND node_dashs ${node_dashs_res})
endif()
