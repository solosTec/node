# top level files
set (node_ipt_gateway)

set (node_ipt_gateway_cpp

	nodes/ipt/gateway/src/main.cpp	
	nodes/ipt/gateway/src/controller.cpp
	nodes/ipt/gateway/src/core.cpp
	nodes/ipt/gateway/src/executor.cpp
	nodes/ipt/gateway/src/data.cpp
	nodes/ipt/gateway/src/attention.cpp
	nodes/ipt/gateway/src/cfg_push.cpp
	nodes/ipt/gateway/src/cfg_ipt.cpp
)

set (node_ipt_gateway_h

	nodes/ipt/gateway/src/controller.h
	nodes/ipt/gateway/src/core.h
	nodes/ipt/gateway/src/executor.h
	nodes/ipt/gateway/src/data.h
	nodes/ipt/gateway/src/attention.h
	nodes/ipt/gateway/src/cfg_push.h
	nodes/ipt/gateway/src/cfg_ipt.h
)

set (node_ipt_gateway_schemes

	nodes/shared/db/db_meta.h
	nodes/shared/db/db_meta.cpp
)

set (node_ipt_gateway_info
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/show_fs_drives.h
	nodes/write_pid.h

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/show_fs_drives.cpp
	nodes/write_pid.cpp
)

set (node_ipt_gateway_tasks

	nodes/ipt/gateway/src/tasks/network.h
	nodes/ipt/gateway/src/tasks/network.cpp
	nodes/ipt/gateway/src/tasks/push_ops.h
	nodes/ipt/gateway/src/tasks/push_ops.cpp
	nodes/ipt/gateway/src/tasks/wired_lmn.cpp
	nodes/ipt/gateway/src/tasks/wireless_lmn.cpp
	nodes/ipt/gateway/src/tasks/wired_lmn.h
	nodes/ipt/gateway/src/tasks/wireless_lmn.h
	nodes/ipt/gateway/src/tasks/gpio.h
	nodes/ipt/gateway/src/tasks/gpio.cpp
	nodes/ipt/gateway/src/tasks/virtual_meter.h
	nodes/ipt/gateway/src/tasks/virtual_meter.cpp
)
	
set (node_ipt_gateway_server

	nodes/ipt/gateway/src/connection.h
	nodes/ipt/gateway/src/connection.cpp
	nodes/ipt/gateway/src/session.h
	nodes/ipt/gateway/src/session.cpp
	nodes/ipt/gateway/src/server.h
	nodes/ipt/gateway/src/server.cpp
)

if(WIN32)

	set (node_ipt_gateway_service
		nodes/ipt/gateway/templates/gateway_create_service.cmd.in
		nodes/ipt/gateway/templates/gateway_delete_service.cmd.in
		nodes/ipt/gateway/templates/gateway_restart_service.cmd.in
		nodes/ipt/gateway/templates/gateway.windows.cgf.in
	)
 
	set (node_ipt_gateway_manifest
		nodes/ipt/gateway/templates/gateway.exe.manifest
	)

	set (node_ipt_gateway_res
		${CMAKE_CURRENT_BINARY_DIR}/gateway.rc 
	)

else()

	set (node_ipt_gateway_service
		nodes/ipt/gateway/templates/gateway.service.in
		nodes/ipt/gateway/templates/gateway.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_ipt_gateway_tasks})
source_group("service" FILES ${node_ipt_gateway_service})
source_group("info" FILES ${node_ipt_gateway_info})
source_group("server" FILES ${node_ipt_gateway_server})
source_group("schemes" FILES ${node_ipt_gateway_schemes})


# define the main program
set (node_ipt_gateway
  ${node_ipt_gateway_cpp}
  ${node_ipt_gateway_h}
  ${node_ipt_gateway_tasks}
  ${node_ipt_gateway_service}
  ${node_ipt_gateway_info}
  ${node_ipt_gateway_server}
  ${node_ipt_gateway_schemes}
)

if(WIN32)
	source_group("manifest" FILES ${node_ipt_gateway_manifest})
	list(APPEND node_ipt_gateway ${node_ipt_gateway_manifest})
	source_group("resources" FILES ${node_ipt_gateway_res})
	list(APPEND node_ipt_gateway ${node_ipt_gateway_res})
endif()

if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	set (node_ipt_gateway_xml
		${PUGIXML_INCLUDE_DIR}/pugixml.hpp
		${PUGIXML_INCLUDE_DIR}/pugixml.cpp
	)
	list(APPEND node_ipt_gateway ${node_ipt_gateway_xml})
	source_group("XML" FILES ${node_ipt_gateway_xml})

endif()
