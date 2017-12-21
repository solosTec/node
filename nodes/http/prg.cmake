# top level files
set (node_http)

set (node_http_cpp

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
 	nodes/http/src/controller.cpp
 	nodes/http/src/listener.cpp
  	nodes/http/src/connections.cpp
 	nodes/http/src/session.cpp
 	nodes/http/src/websocket.cpp
 	nodes/http/src/handle_request.hpp
 	nodes/http/src/path_cat.cpp
 	nodes/http/src/mime_type.cpp
 	nodes/http/src/auth.cpp
 	nodes/http/src/mail_config.cpp
 	nodes/http/src/main.cpp
#  	nodes/http/src/advanced_server.cpp
	
)

set (node_http_h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/http/src/controller.h
	nodes/http/src/listener.h
	nodes/http/src/connections.h
	nodes/http/src/session.h
	nodes/http/src/websocket.h
	nodes/http/src/auth.h
	nodes/http/src/mail_config.h
	nodes/http/src/path_cat.h
	nodes/http/src/mime_type.h

)

set (node_http_tasks
)

set (node_http_res
)
	

# if(WIN32)
# 
# 	set (dashboard_service
# 		src/main/templates/create_dashboard_srv.bat.in
# 		src/main/templates/delete_dashboard_srv.bat.in
# 		src/main/templates/restart_dashboard_srv.bat.in
# 		src/main/templates/dashboard.windows.cgf.in
# 	)
# 
# else()
# 
# 	set (dashboard_service
# 		src/main/templates/dashboard.service.in
# 		src/main/templates/dashboard.windows.cgf.in
# 	)
# 
# endif()

source_group("tasks" FILES ${node_http_tasks})
source_group("resources" FILES ${node_http_res})
# source_group("service" FILES ${node_http_service})


# define the main program
set (node_http
  ${node_http_cpp}
  ${node_http_h}
  ${node_http_tasks}
  ${node_http_res}
#   ${node_http_service}
)

