# top level files
set (node_ipt_stress)

set (node_ipt_stress_cpp

	nodes/ipt/stress/src/main.cpp	
	nodes/ipt/stress/src/controller.cpp
)

set (node_ipt_stress_h

	nodes/ipt/stress/src/controller.h

)

set (node_ipt_stress_info
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

set (node_ipt_stress_tasks
	nodes/ipt/stress/src/tasks/sender.h
	nodes/ipt/stress/src/tasks/sender.cpp
	nodes/ipt/stress/src/tasks/receiver.h
	nodes/ipt/stress/src/tasks/receiver.cpp
)

set (node_ipt_stress_res
)
	
if(WIN32)

	set (node_ipt_stress_service
		nodes/ipt/stress/templates/stress_create_service.cmd.in
		nodes/ipt/stress/templates/stress_delete_service.cmd.in
		nodes/ipt/stress/templates/stress_restart_service.cmd.in
		nodes/ipt/stress/templates/stress.windows.cgf.in
	)
 
else()

	set (node_ipt_stress_service
		nodes/ipt/stress/templates/stress.service.in
		nodes/ipt/stress/templates/stress.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_ipt_stress_tasks})
source_group("resources" FILES ${node_ipt_stress_res})
source_group("service" FILES ${node_ipt_stress_service})
source_group("info" FILES ${node_ipt_stress_info})


# define the main program
set (node_ipt_stress
  ${node_ipt_stress_cpp}
  ${node_ipt_stress_h}
  ${node_ipt_stress_tasks}
  ${node_ipt_stress_res}
  ${node_ipt_stress_service}
  ${node_ipt_stress_info}
)

if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	set (node_ipt_stress_xml
		${PUGIXML_INCLUDE_DIR}/pugixml.hpp
		${PUGIXML_INCLUDE_DIR}/pugixml.cpp
	)
	list(APPEND node_ipt_stress ${node_ipt_stress_xml})
	source_group("XML" FILES ${node_ipt_stress_xml})

endif()
