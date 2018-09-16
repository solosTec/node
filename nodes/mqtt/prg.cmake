# top level files
set (node_mqtt)

set (node_mqtt_cpp

	nodes/mqtt/src/main.cpp	
	nodes/mqtt/src/controller.cpp
	nodes/mqtt/src/server.cpp
)

set (node_mqtt_h

	nodes/mqtt/src/controller.h
	nodes/mqtt/src/server.h
)

set (node_mqtt_info
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


set (node_mqtt_tasks
	nodes/mqtt/src/tasks/cluster.h
	nodes/mqtt/src/tasks/cluster.cpp
)

set (node_mqtt_res
)
	
if(WIN32)

	set (node_mqtt_service
		nodes/mqtt/templates/mqtt_create_service.cmd.in
		nodes/mqtt/templates/mqtt_delete_service.cmd.in
		nodes/mqtt/templates/mqtt_restart_service.cmd.in
		nodes/mqtt/templates/mqtt.windows.cgf.in
	)
 
else()

	set (node_mqtt_service
		nodes/mqtt/templates/mqtt.service.in
		nodes/mqtt/templates/mqtt.linux.cgf.in
	)

endif()


source_group("tasks" FILES ${node_mqtt_tasks})
source_group("resources" FILES ${node_mqtt_res})
source_group("service" FILES ${node_mqtt_service})
source_group("info" FILES ${node_mqtt_info})


# define the main program
set (node_mqtt
  ${node_mqtt_cpp}
  ${node_mqtt_h}
  ${node_mqtt_info}
  ${node_mqtt_tasks}
  ${node_mqtt_res}
  ${node_mqtt_service}
)

