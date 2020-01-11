# top level files
set (node_dash)
set (node_dashs)

set (node_dash_cpp

	nodes/admin/dash/src/main.cpp	
	nodes/admin/dash/src/controller.cpp
)

set (node_dash_h

	nodes/admin/dash/src/controller.h

)

set (node_dash_tasks
	nodes/admin/dash/src/tasks/cluster.h
	nodes/admin/dash/src/tasks/cluster.cpp
	nodes/admin/dash/src/tasks/system.h
	nodes/admin/dash/src/tasks/system.cpp
)

set (node_admin_shared
	nodes/admin/shared/src/sync_db.h
	nodes/admin/shared/src/sync_db.cpp
	nodes/admin/shared/src/dispatcher.h
	nodes/admin/shared/src/dispatcher.cpp
	nodes/admin/shared/src/forwarder.h
	nodes/admin/shared/src/forwarder.cpp
	nodes/admin/shared/src/form_data.h
	nodes/admin/shared/src/form_data.cpp

	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h

	src/main/include/smf/shared/ctl.h
	nodes/shared/sys/ctl.cpp

)

set (node_dash_schemes

	src/main/include/smf/shared/db_schemes.h
	nodes/shared/db/db_schemes.cpp
)
	
if(WIN32)

	set (node_dash_service
		nodes/admin/templates/dash_create_service.cmd.in
		nodes/admin/templates/dash_delete_service.cmd.in
		nodes/admin/templates/dash_restart_service.cmd.in
		nodes/admin/templates/dash.windows.cgf.in
	)
 
 	set (node_dash_res
		${CMAKE_CURRENT_BINARY_DIR}/dash.rc 
		src/main/resources/logo.ico
		nodes/admin/templates/dash.exe.manifest
	)

else()

	set (node_dash_service
		nodes/admin/templates/dash.service.in
		nodes/admin/templates/dash.linux.cgf.in
	)

endif()

source_group("dash/tasks" FILES ${node_dash_tasks})
source_group("dash/service" FILES ${node_dash_service})
source_group("dash/shared" FILES ${node_admin_shared})
source_group("dash/schemes" FILES ${node_dash_schemes})


# define the main program
set (node_dash
  ${node_dash_cpp}
  ${node_dash_h}
  ${node_dash_tasks}
  ${node_dash_service}
  ${node_admin_shared}
  ${node_dash_schemes}
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

	nodes/admin/dashs/src/main.cpp	
	nodes/admin/dashs/src/controller.cpp
)

set (node_dashs_h

	nodes/admin/dashs/src/controller.h

)

set (node_dashs_tasks
	nodes/admin/dashs/src/tasks/cluster.h
	nodes/admin/dashs/src/tasks/cluster.cpp
	nodes/admin/dashs/src/tasks/system.h
	nodes/admin/dashs/src/tasks/system.cpp
)

set (node_dashs_schemes

	src/main/include/smf/shared/db_schemes.h
	nodes/shared/db/db_schemes.cpp
)
	
if(WIN32)

	set (node_dashs_service
		nodes/admin/templates/dashs_create_service.cmd.in
		nodes/admin/templates/dashs_delete_service.cmd.in
		nodes/admin/templates/dashs_restart_service.cmd.in
		nodes/admin/templates/dashs.windows.cgf.in
	)

 	set (node_dashs_res
		${CMAKE_CURRENT_BINARY_DIR}/dashs.rc 
		src/main/resources/logo.ico
		nodes/admin/templates/dashs.exe.manifest
	)

else()

	set (node_dashs_service
		nodes/admin/templates/dashs.service.in
		nodes/admin/templates/dashs.linux.cgf.in
	)

endif()

source_group("dashs/tasks" FILES ${node_dashs_tasks})
source_group("dashs/service" FILES ${node_dashs_service})
source_group("dashs/schemes" FILES ${node_dashs_schemes})


# define the main program
set (node_dashs
  ${node_dashs_cpp}
  ${node_dashs_h}
  ${node_dashs_tasks}
  ${node_dashs_service}
  ${node_admin_shared}
  ${node_dashs_schemes}
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
