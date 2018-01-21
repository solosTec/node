# top level files
set (node_modem)

set (node_modem_cpp

	nodes/modem/src/main.cpp	
	nodes/modem/src/controller.cpp
)

set (node_modem_h

	nodes/modem/src/controller.h

)

set (node_modem_info
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

set (node_modem_tasks
)

set (node_modem_res
)
	
if(WIN32)

	set (node_modem_service
		nodes/modem/templates/modem_create_service.bat.in
		nodes/modem/templates/modem_delete_service.bat.in
		nodes/modem/templates/modem_restart_service.bat.in
		nodes/modem/templates/modem.windows.cgf.in
	)
 
else()

	set (node_modem_service
		nodes/modem/templates/modem.service.in
		nodes/modem/templates/modem.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_modem_tasks})
source_group("resources" FILES ${node_modem_res})
source_group("service" FILES ${node_modem_service})
source_group("info" FILES ${node_modem_info})


# define the main program
set (node_modem
  ${node_modem_cpp}
  ${node_modem_h}
  ${node_modem_tasks}
  ${node_modem_res}
  ${node_modem_service}
  ${node_modem_info}
)

