# top level files
set (task_tsdb)

set (task_tsdb_cpp

	src/main.cpp	
	src/controller.cpp
	src/dispatcher.cpp
)

set (task_tsdb_h

	src/controller.h
	src/dispatcher.h
)

set (task_tsdb_schemes

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/db_schemes.h
	${CMAKE_SOURCE_DIR}/nodes/shared/db/db_schemes.cpp
)

set (task_tsdb_shared
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/tasks/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/tasks/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h
#	${CMAKE_SOURCE_DIR}/nodes/shared/sys/ctl.cpp

)

if (UNIX)
	list(APPEND task_tsdb_shared ${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/write_pid.h)
	list(APPEND task_tsdb_shared ${CMAKE_SOURCE_DIR}/nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (task_tsdb_tasks
	src/tasks/cluster.h
	src/tasks/cluster.cpp
	src/tasks/single.h
	src/tasks/single.cpp
	src/tasks/multiple.h
	src/tasks/multiple.cpp
	src/tasks/line_protocol.h
	src/tasks/line_protocol.cpp
)
	
if(WIN32)

	set (task_tsdb_service
		templates/tsdb_create_service.cmd.in
		templates/tsdb_delete_service.cmd.in
		templates/tsdb_restart_service.cmd.in
		templates/tsdb.windows.cgf.in
	)
 
 	set (task_tsdb_manifest
		templates/tsdb.exe.manifest
	)

 	set (task_tsdb_res
		${CMAKE_BINARY_DIR}/tsdb.rc 
		${CMAKE_SOURCE_DIR}/src/main/resources/logo.ico
	)

else()

	set (task_tsdb_service
		templates/tsdb.service.in
		templates/tsdb.linux.cgf.in
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
