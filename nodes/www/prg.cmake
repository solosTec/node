# top level files
set (node_http)
set (node_https)

set (node_http_cpp

 	nodes/www/http/src/controller.cpp
 	nodes/www/http/src/logic.cpp
# 	nodes/www/http/src/mail_config.cpp
 	nodes/www/http/src/main.cpp

)

set (node_http_h

	nodes/www/http/src/controller.h
	nodes/www/http/src/logic.h
#	nodes/www/http/src/mail_config.h

)

set (node_www_info
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
)

if(WIN32)

	set (node_http_service
		nodes/www/templates/http_create_service.cmd.in
		nodes/www/templates/http_delete_service.cmd.in
		nodes/www/templates/http_restart_service.cmd.in
		nodes/www/templates/http.windows.cgf.in
	)
 
else()

	set (node_http_service
		nodes/www/templates/http.service.in
		nodes/www/templates/http.linux.cgf.in
	)

endif()

source_group("http/service" FILES ${node_http_service})
source_group("www/info" FILES ${node_www_info})


# define the main program
set (node_http
  ${node_http_cpp}
  ${node_http_h}
  ${node_http_service}
  ${node_www_info}
)

if(WIN32)
	set (node_http_res
		${CMAKE_CURRENT_BINARY_DIR}/http.rc 
		src/main/resources/logo.ico
		nodes/www/templates/http.exe.manifest
	)
	source_group("resources" FILES ${node_http_res})
	list(APPEND node_http ${node_http_res})
endif()


# --------------------------------------------------------------------+

set (node_https_cpp

 	nodes/www/https/src/controller.cpp
 	nodes/www/https/src/logic.cpp
 	nodes/www/https/src/main.cpp

)

set (node_https_h

	nodes/www/https/src/controller.h
	nodes/www/https/src/logic.h

)

if(WIN32)

	set (node_https_service
		nodes/www/templates/https_create_service.cmd.in
		nodes/www/templates/https_delete_service.cmd.in
		nodes/www/templates/https_restart_service.cmd.in
		nodes/www/templates/https.windows.cgf.in
	)
 
	set (node_https_res
		${CMAKE_CURRENT_BINARY_DIR}/https.rc 
		src/main/resources/logo.ico
		nodes/www/templates/https.exe.manifest
	)

else()

	set (node_https_service
		nodes/www/templates/https.service.in
		nodes/www/templates/https.linux.cgf.in
	)

endif()

source_group("service" FILES ${node_https_service})


# define the main program
set (node_https
  ${node_https_cpp}
  ${node_https_h}
  ${node_https_service}
  ${node_www_info}
)

if(WIN32)
	source_group("resources" FILES ${node_https_res})
	list(APPEND node_https ${node_https_res})
endif()
