# top level files
set (node_ipt_emitter)

set (node_ipt_emitter_cpp

	nodes/ipt/emitter/src/main.cpp	
	nodes/ipt/emitter/src/controller.cpp
)

set (node_ipt_emitter_h
	nodes/ipt/emitter/src/controller.h
)

set (node_ipt_emitter_shared
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h	

	src/main/include/smf/shared/ctl.h
)

if (UNIX)
	list(APPEND node_ipt_emitter_shared src/main/include/smf/shared/write_pid.h)
endif(UNIX)

set (node_ipt_emitter_tasks
	nodes/ipt/emitter/src/tasks/network.h
	nodes/ipt/emitter/src/tasks/network.cpp
)
	
if(WIN32)

	set (node_ipt_emitter_service
		nodes/ipt/emitter/templates/emitter_create_service.cmd.in
		nodes/ipt/emitter/templates/emitter_delete_service.cmd.in
		nodes/ipt/emitter/templates/emitter_restart_service.cmd.in
		nodes/ipt/emitter/templates/emitter.windows.cgf.in
	)
 
else()

	set (node_ipt_emitter_service
		nodes/ipt/emitter/templates/emitter.service.in
		nodes/ipt/emitter/templates/emitter.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_ipt_emitter_tasks})
source_group("service" FILES ${node_ipt_emitter_service})
source_group("info" FILES ${node_ipt_emitter_shared})


# define the main program
set (node_ipt_emitter
  ${node_ipt_emitter_cpp}
  ${node_ipt_emitter_h}
  ${node_ipt_emitter_tasks}
  ${node_ipt_emitter_service}
  ${node_ipt_emitter_shared}
)

