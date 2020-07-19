# top level files
set (node_ipt_stress)

set (node_ipt_stress_cpp

	src/main.cpp	
	src/controller.cpp
)

set (node_ipt_stress_h

	src/controller.h

)

set (node_ipt_stress_shared
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h

#	${CMAKE_SOURCE_DIR}/nodes/print_build_info.cpp
#	${CMAKE_SOURCE_DIR}/nodes/print_version_info.cpp
#	${CMAKE_SOURCE_DIR}/nodes/set_start_options.cpp
#	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.cpp

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h
	${CMAKE_SOURCE_DIR}/nodes/shared/sys/ctl.cpp

)

set (node_ipt_stress_tasks
	src/tasks/sender.h
	src/tasks/sender.cpp
	src/tasks/receiver.h
	src/tasks/receiver.cpp
)

if (UNIX)
	list(APPEND node_ipt_stress_shared src/main/include/smf/shared/write_pid.h)
	list(APPEND node_ipt_stress_shared nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (node_ipt_stress_res
)
	
if(WIN32)

	set (node_ipt_stress_service
		templates/stress_create_service.cmd.in
		templates/stress_delete_service.cmd.in
		templates/stress_restart_service.cmd.in
		templates/stress.windows.cgf.in
	)
 
else()

	set (node_ipt_stress_service
		templates/stress.service.in
		templates/stress.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_ipt_stress_tasks})
source_group("resources" FILES ${node_ipt_stress_res})
source_group("service" FILES ${node_ipt_stress_service})
source_group("shared" FILES ${node_ipt_stress_shared})


# define the main program
set (node_ipt_stress
  ${node_ipt_stress_cpp}
  ${node_ipt_stress_h}
  ${node_ipt_stress_tasks}
  ${node_ipt_stress_res}
  ${node_ipt_stress_service}
  ${node_ipt_stress_shared}
)

if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	set (node_ipt_stress_xml
		${PUGIXML_INCLUDE_DIR}/pugixml.hpp
		${PUGIXML_INCLUDE_DIR}/pugixml.cpp
	)
	list(APPEND node_ipt_stress ${node_ipt_stress_xml})
	source_group("XML" FILES ${node_ipt_stress_xml})

endif()
