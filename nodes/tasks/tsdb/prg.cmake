# top level files
set (task_tsdb)
set (task_tsdb_root nodes/tasks/tsdb)

set (task_tsdb_cpp

	${task_tsdb_root}/src/main.cpp	
	${task_tsdb_root}/src/controller.cpp
	${task_tsdb_root}/src/dispatcher.cpp
)

set (task_tsdb_h

	${task_tsdb_root}/src/controller.h
	${task_tsdb_root}/src/dispatcher.h
)

set (task_tsdb_schemes

	src/main/include/smf/shared/db_schemes.h
	nodes/shared/db/db_schemes.cpp
)

set (task_tsdb_shared
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/tasks/print_version_info.h
	nodes/tasks/set_start_options.h
	nodes/show_ip_address.h

	nodes/print_build_info.cpp
	nodes/tasks/print_version_info.cpp
	nodes/tasks/set_start_options.cpp
	nodes/show_ip_address.cpp

	src/main/include/smf/shared/ctl.h
	nodes/shared/sys/ctl.cpp

)

if (UNIX)
	list(APPEND task_tsdb_shared src/main/include/smf/shared/write_pid.h)
	list(APPEND task_tsdb_shared nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (task_tsdb_tasks
	${task_tsdb_root}/src/tasks/cluster.h
	${task_tsdb_root}/src/tasks/cluster.cpp
	${task_tsdb_root}/src/tasks/single.h
	${task_tsdb_root}/src/tasks/single.cpp
	${task_tsdb_root}/src/tasks/multiple.h
	${task_tsdb_root}/src/tasks/multiple.cpp
	${task_tsdb_root}/src/tasks/line_protocol.h
	${task_tsdb_root}/src/tasks/line_protocol.cpp
)
	
if(WIN32)

	set (task_tsdb_service
		${task_tsdb_root}/templates/tsdb_create_service.cmd.in
		${task_tsdb_root}/templates/tsdb_delete_service.cmd.in
		${task_tsdb_root}/templates/tsdb_restart_service.cmd.in
		${task_tsdb_root}/templates/tsdb.windows.cgf.in
	)
 
 	set (task_tsdb_manifest
		${task_tsdb_root}/templates/tsdb.exe.manifest
	)

 	set (task_tsdb_res
		${CMAKE_CURRENT_BINARY_DIR}/tsdb.rc 
		src/main/resources/logo.ico
	)

else()

	set (task_tsdb_service
		${task_tsdb_root}/templates/tsdb.service.in
		${task_tsdb_root}/templates/tsdb.linux.cgf.in
	)

endif()


source_group("tasks" FILES ${task_tsdb_tasks})
source_group("service" FILES ${task_tsdb_service})
source_group("shared" FILES ${task_tsdb_shared})
source_group("schemes" FILES ${task_tsdb_schemes})


# define the main program
set (task_tsdb
  ${task_tsdb_cpp}
  ${task_tsdb_h}
  ${task_tsdb_shared}
  ${task_tsdb_tasks}
  ${task_tsdb_service}
  ${task_tsdb_schemes}
)

if(WIN32)
	source_group("manifest" FILES ${task_tsdb_manifest})
	list(APPEND task_tsdb ${task_tsdb_manifest})
	source_group("resources" FILES ${task_tsdb_res})
	list(APPEND task_tsdb ${task_tsdb_res})
endif()
