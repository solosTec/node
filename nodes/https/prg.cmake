# top level files
set (node_https)

set (node_https_cpp

 	nodes/https/src/controller.cpp
 	nodes/https/src/server_certificate.hpp
 	nodes/https/src/main.cpp

)

set (node_https_h

	nodes/https/src/controller.h

)

set (node_https_info
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

set (node_https_tasks
)

set (node_https_res
)
	

if(WIN32)

	set (node_https_service
		nodes/https/templates/https_create_service.bat.in
		nodes/https/templates/https_delete_service.bat.in
		nodes/https/templates/https_restart_service.bat.in
		nodes/https/templates/https.windows.cgf.in
	)
 
else()

	set (node_https_service
		nodes/https/templates/https.service.in
		nodes/https/templates/https.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_https_tasks})
source_group("resources" FILES ${node_https_res})
source_group("service" FILES ${node_https_service})
source_group("resources" FILES ${node_https_info})


# define the main program
set (node_https
  ${node_https_cpp}
  ${node_https_h}
  ${node_https_tasks}
  ${node_https_res}
  ${node_https_service}
  ${node_https_info}
)

