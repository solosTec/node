# top level files
set (node_dashs)

set (node_dashs_cpp

	nodes/dashs/src/main.cpp	
	nodes/dashs/src/controller.cpp
)

set (node_dashs_h

	nodes/dashs/src/controller.h

)

set (node_dashs_tasks
	nodes/dashs/src/tasks/cluster.h
	nodes/dashs/src/tasks/cluster.cpp
	nodes/dashs/src/tasks/system.h
	nodes/dashs/src/tasks/system.cpp
)

set (node_dashs_shared
	nodes/dash_shared/src/sync_db.h
	nodes/dash_shared/src/sync_db.cpp
	nodes/dash_shared/src/dispatcher.h
	nodes/dash_shared/src/dispatcher.cpp
)

#
#	same as for dash (HTTP)
#
set (node_dashs_res
	nodes/dash/htdocs/index.html
	nodes/dash/htdocs/config.system.html
	nodes/dash/htdocs/config.device.html
	nodes/dash/htdocs/config.gateway.html
	nodes/dash/htdocs/config.lora.html
	nodes/dash/htdocs/config.meter.html
	nodes/dash/htdocs/config.upload.html
	nodes/dash/htdocs/config.download.html
	nodes/dash/htdocs/status.session.html
	nodes/dash/htdocs/status.system.html
	nodes/dash/htdocs/status.targets.html
	nodes/dash/htdocs/status.connections.html
	nodes/dash/htdocs/monitor.msg.html
)

set (node_dashs_info
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

	
if(WIN32)

	set (node_dashs_service
		nodes/dashs/templates/dashs_create_service.cmd.in
		nodes/dashs/templates/dashs_delete_service.cmd.in
		nodes/dashs/templates/dashs_restart_service.cmd.in
		nodes/dashs/templates/dashs.windows.cgf.in
	)
 
else()

	set (node_dashs_service
		nodes/dashs/templates/dashs.service.in
		nodes/dashs/templates/dashs.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_dashs_tasks})
source_group("resources" FILES ${node_dashs_res})
source_group("service" FILES ${node_dashs_service})
source_group("info" FILES ${node_dashs_info})
source_group("shared" FILES ${node_dashs_shared})


# define the main program
set (node_dashs
  ${node_dashs_cpp}
  ${node_dashs_h}
  ${node_dashs_tasks}
  ${node_dashs_res}
  ${node_dashs_service}
  ${node_dashs_info}
  ${node_dashs_shared}
)

if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	set (node_dashs_xml
		${PUGIXML_INCLUDE_DIR}/pugixml.hpp
		${PUGIXML_INCLUDE_DIR}/pugixml.cpp
	)
	list(APPEND node_dashs ${node_dashs_xml})
	source_group("XML" FILES ${node_dashs_xml})

endif()
