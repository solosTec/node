# top level files
set (node_http)

set (node_http_cpp

 	nodes/http/src/controller.cpp
 	nodes/http/src/logic.cpp
# 	nodes/http/src/mail_config.cpp
 	nodes/http/src/main.cpp

)

set (node_http_h

	nodes/http/src/controller.h
	nodes/http/src/logic.h
#	nodes/http/src/mail_config.h

)

set (node_http_info
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

if(WIN32)

	set (node_http_service
		nodes/http/templates/http_create_service.cmd.in
		nodes/http/templates/http_delete_service.cmd.in
		nodes/http/templates/http_restart_service.cmd.in
		nodes/http/templates/http.windows.cgf.in
	)
 
else()

	set (node_http_service
		nodes/http/templates/http.service.in
		nodes/http/templates/http.linux.cgf.in
	)

endif()

source_group("service" FILES ${node_http_service})
source_group("info" FILES ${node_http_info})


# define the main program
set (node_http
  ${node_http_cpp}
  ${node_http_h}
  ${node_http_service}
  ${node_http_info}
)

if(WIN32)
	set (node_http_res
		${CMAKE_CURRENT_BINARY_DIR}/http.rc 
		src/main/resources/logo.ico
		nodes/http/templates/http.exe.manifest
	)
	source_group("resources" FILES ${node_http_res})
	list(APPEND node_http ${node_http_res})
endif()
