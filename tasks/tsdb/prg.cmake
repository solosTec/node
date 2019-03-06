# top level files
set (task_tsdb)

set (task_tsdb_cpp

	tasks/tsdb/src/main.cpp	
	tasks/tsdb/src/controller.cpp
	tasks/tsdb/src/dispatcher.cpp
)

set (task_tsdb_h

	tasks/tsdb/src/controller.h
	tasks/tsdb/src/dispatcher.h
)

set (task_tsdb_schemes

	nodes/shared/db/db_schemes.h
	nodes/shared/db/db_schemes.cpp
)

set (task_tsdb_info
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	tasks/print_version_info.h
	tasks/set_start_options.h
	nodes/show_ip_address.h
	nodes/write_pid.h

	nodes/print_build_info.cpp
	tasks/print_version_info.cpp
	tasks/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/write_pid.cpp
)


set (task_tsdb_tasks
	tasks/tsdb/src/tasks/cluster.h
	tasks/tsdb/src/tasks/cluster.cpp
	tasks/tsdb/src/tasks/single.h
	tasks/tsdb/src/tasks/single.cpp
	tasks/tsdb/src/tasks/multiple.h
	tasks/tsdb/src/tasks/multiple.cpp
	tasks/tsdb/src/tasks/line_protocol.h
	tasks/tsdb/src/tasks/line_protocol.cpp
)
	
if(WIN32)

	set (task_tsdb_service
		tasks/tsdb/templates/tsdb_create_service.cmd.in
		tasks/tsdb/templates/tsdb_delete_service.cmd.in
		tasks/tsdb/templates/tsdb_restart_service.cmd.in
		tasks/tsdb/templates/tsdb.windows.cgf.in
	)
 
 	set (task_tsdb_manifest
		tasks/tsdb/templates/tsdb.exe.manifest
	)

 	set (task_tsdb_res
		${CMAKE_CURRENT_BINARY_DIR}/tsdb.rc 
		src/main/resources/logo.ico
	)

else()

	set (task_tsdb_service
		tasks/tsdb/templates/tsdb.service.in
		tasks/tsdb/templates/tsdb.linux.cgf.in
	)

endif()


source_group("tasks" FILES ${task_tsdb_tasks})
source_group("service" FILES ${task_tsdb_service})
source_group("info" FILES ${task_tsdb_info})
source_group("schemes" FILES ${task_tsdb_schemes})


# define the main program
set (task_tsdb
  ${task_tsdb_cpp}
  ${task_tsdb_h}
  ${task_tsdb_info}
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
