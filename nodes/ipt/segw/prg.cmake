# top level files
set (node_ipt_segw)

set (node_ipt_segw_cpp

	nodes/ipt/segw/src/main.cpp	
)

set (node_ipt_segw_h

)

set (node_ipt_segw_schemes

	src/main/include/smf/shared/db_meta.h
	nodes/shared/db/db_meta.cpp
	src/main/include/smf/shared/db_cfg.h
	nodes/shared/db/db_cfg.cpp
)

set (node_ipt_segw_shared
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/show_fs_drives.h

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/show_fs_drives.cpp

	src/main/include/smf/shared/ctl.h
	nodes/shared/sys/ctl.cpp
)

if (UNIX)
	list(APPEND node_ipt_segw_shared src/main/include/smf/shared/write_pid.h)
	list(APPEND node_ipt_segw_shared nodes/shared/sys/write_pid.cpp)
endif(UNIX)


set (node_ipt_segw_tasks

)
	
set (node_ipt_segw_server

)

if(WIN32)

	set (node_ipt_segw_service
		nodes/ipt/segw/templates/segw_create_service.cmd.in
		nodes/ipt/segw/templates/segw_delete_service.cmd.in
		nodes/ipt/segw/templates/segw_restart_service.cmd.in
		nodes/ipt/segw/templates/segw.windows.cgf.in
	)
 
	set (node_ipt_segw_manifest
		nodes/ipt/segw/templates/segw.exe.manifest
	)

	set (node_ipt_segw_res
		${CMAKE_CURRENT_BINARY_DIR}/segw.rc 
	)

else()

	set (node_ipt_segw_service
		nodes/ipt/segw/templates/segw.service.in
		nodes/ipt/segw/templates/segw.linux.cgf.in
	)

endif()

source_group("tasks" FILES ${node_ipt_segw_tasks})
source_group("service" FILES ${node_ipt_segw_service})
source_group("shared" FILES ${node_ipt_segw_shared})
source_group("server" FILES ${node_ipt_segw_server})
source_group("schemes" FILES ${node_ipt_segw_schemes})


# define the main program
set (node_ipt_segw
  ${node_ipt_segw_cpp}
  ${node_ipt_segw_h}
  ${node_ipt_segw_tasks}
  ${node_ipt_segw_service}
  ${node_ipt_segw_shared}
  ${node_ipt_segw_server}
  ${node_ipt_segw_schemes}
)

if(WIN32)
	source_group("manifest" FILES ${node_ipt_segw_manifest})
	list(APPEND node_ipt_segw ${node_ipt_segw_manifest})
	source_group("resources" FILES ${node_ipt_segw_res})
	list(APPEND node_ipt_segw ${node_ipt_segw_res})
endif()

