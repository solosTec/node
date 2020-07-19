# top level files
set (node_http)
set (node_https)

set (node_http_cpp

 	http/src/controller.cpp
 	http/src/logic.cpp
 	http/src/main.cpp

)

set (node_http_h

	http/src/controller.h
	http/src/logic.h

)

set (node_www_info
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h
)

if(WIN32)

	set (node_http_service
		templates/http_create_service.cmd.in
		templates/http_delete_service.cmd.in
		templates/http_restart_service.cmd.in
		templates/http.windows.cgf.in
	)
 
else()

	set (node_http_service
		templates/http.service.in
		templates/http.linux.cgf.in
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
		${CMAKE_BINARY_DIR}/http.rc 
		${CMAKE_SOURCE_DIR}/src/main/resources/logo.ico
		templates/http.exe.manifest
	)
	source_group("resources" FILES ${node_http_res})
	list(APPEND node_http ${node_http_res})
endif()


# --------------------------------------------------------------------+

set (node_https_cpp

 	https/src/controller.cpp
 	https/src/logic.cpp
 	https/src/main.cpp

)

set (node_https_h

	https/src/controller.h
	https/src/logic.h

)

if(WIN32)

	set (node_https_service
		templates/https_create_service.cmd.in
		templates/https_delete_service.cmd.in
		templates/https_restart_service.cmd.in
		templates/https.windows.cgf.in
	)
 
	set (node_https_res
		${CMAKE_BINARY_DIR}/https.rc 
		${CMAKE_SOURCE_DIR}/src/main/resources/logo.ico
		templates/https.exe.manifest
	)

else()

	set (node_https_service
		templates/https.service.in
		templates/https.linux.cgf.in
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
