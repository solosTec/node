# top level files
set (node_mqtt)

set (node_mqtt_cpp

	src/main.cpp	
	src/controller.cpp
	src/server.cpp
)

set (node_mqtt_h

	src/controller.h
	src/server.h
)

set (node_mqtt_shared
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h
)


if (UNIX)
	list(APPEND node_mqtt_shared src/main/include/smf/shared/write_pid.h)
	list(APPEND node_mqtt_shared nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (node_mqtt_tasks
	src/tasks/cluster.h
	src/tasks/cluster.cpp
)

#
# include 3rdparty sources from https://github.com/redboltz/mqtt_cpp.git
#
set(MQTT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/3party/mqtt)

set (node_mqtt_srv
	${MQTT_INCLUDE_DIR}/server.hpp
	${MQTT_INCLUDE_DIR}/connect_flags.hpp
	${MQTT_INCLUDE_DIR}/connect_return_code.hpp
	${MQTT_INCLUDE_DIR}/control_packet_type.hpp
	${MQTT_INCLUDE_DIR}/encoded_length.hpp
	${MQTT_INCLUDE_DIR}/exception.hpp
	${MQTT_INCLUDE_DIR}/fixed_header.hpp
	${MQTT_INCLUDE_DIR}/hexdump.hpp
	${MQTT_INCLUDE_DIR}/publish.hpp
	${MQTT_INCLUDE_DIR}/qos.hpp
	${MQTT_INCLUDE_DIR}/remaining_length.hpp
	${MQTT_INCLUDE_DIR}/session_present.hpp
	${MQTT_INCLUDE_DIR}/str_connect_return_code.hpp
	${MQTT_INCLUDE_DIR}/str_qos.hpp
	${MQTT_INCLUDE_DIR}/utf8encoded_strings.hpp
	${MQTT_INCLUDE_DIR}/will.hpp
)
	
if(WIN32)

	set (node_mqtt_service
		templates/mqtt_create_service.cmd.in
		templates/mqtt_delete_service.cmd.in
		templates/mqtt_restart_service.cmd.in
		templates/mqtt.windows.cgf.in
	)
 
else()

	set (node_mqtt_service
		templates/mqtt.service.in
		templates/mqtt.linux.cgf.in
	)

endif()


source_group("tasks" FILES ${node_mqtt_tasks})
source_group("service" FILES ${node_mqtt_service})
source_group("shared" FILES ${node_mqtt_shared})
source_group("mqtt" FILES ${node_mqtt_srv})


# define the main program
set (node_mqtt
  ${node_mqtt_cpp}
  ${node_mqtt_h}
  ${node_mqtt_shared}
  ${node_mqtt_tasks}
  ${node_mqtt_srv}
  ${node_mqtt_service}
)

