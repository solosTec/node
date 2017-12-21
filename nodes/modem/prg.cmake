# top level files
set (node_modem)

set (node_modem_cpp

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/modem/src/main.cpp	
	nodes/modem/src/controller.cpp
)

set (node_modem_h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/modem/src/controller.h

)

set (node_modem_tasks
)

set (node_modem_res
)
	
# if(WIN32)
# 
# 	set (modemboard_service
# 		src/main/templates/create_modem_srv.bat.in
# 		src/main/templates/delete_modem_srv.bat.in
# 		src/main/templates/restart_modem_srv.bat.in
# 		src/main/templates/modem.windows.cgf.in
# 	)
# 
# else()
# 
# 	set (modemboard_service
# 		src/main/templates/modem.service.in
# 		src/main/templates/modem.windows.cgf.in
# 	)
# 
# endif()

source_group("tasks" FILES ${node_modem_tasks})
source_group("resources" FILES ${node_modem_res})
# source_group("service" FILES ${node_modem_service})


# define the main program
set (node_modem
  ${node_modem_cpp}
  ${node_modem_h}
  ${node_modem_tasks}
  ${node_modem_res}
#   ${node_modem_service}
)

