# top level files
set (task_stat)

set (task_stat_cpp

	src/main.cpp	
	src/controller.cpp
)

set (task_stat_h

	src/controller.h
)

set (task_stat_schemes

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/db_meta.h
	${CMAKE_SOURCE_DIR}/nodes/shared/db/db_meta.cpp
)

set (task_stat_shared
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/tasks/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/tasks/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h
)

if (UNIX)
	list(APPEND task_stat_shared ${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/write_pid.h)
	list(APPEND task_stat_shared ${CMAKE_SOURCE_DIR}/nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (task_stat_tasks
	src/tasks/cluster.h
	src/tasks/cluster.cpp
)
	
if(WIN32)

	set (task_stat_service
		templates/stat_create_service.cmd.in
		templates/stat_delete_service.cmd.in
		templates/stat_restart_service.cmd.in
		templates/stat.windows.cgf.in
	)
 
 	set (task_stat_manifest
		templates/stat.exe.manifest
	)

 	set (task_stat_res
		${CMAKE_BINARY_DIR}/stat.rc 
		${CMAKE_SOURCE_DIR}/src/main/resources/logo.ico
	)

else()

	set (task_stat_service
		templates/stat.service.in
		templates/stat.linux.cgf.in
	)

endif()


source_group("tasks" FILES ${task_stat_tasks})
source_group("service" FILES ${task_stat_service})
source_group("info" FILES ${task_stat_shared})
source_group("schemes" FILES ${task_stat_schemes})


# define the main program
set (task_stat
  ${task_stat_cpp}
  ${task_stat_h}
  ${task_stat_shared}
  ${task_stat_tasks}
  ${task_stat_service}
  ${task_stat_schemes}
)

if(WIN32)
	source_group("manifest" FILES ${task_stat_manifest})
	list(APPEND task_stat ${task_stat_manifest})
	source_group("resources" FILES ${task_stat_res})
	list(APPEND task_stat ${task_stat_res})
endif()
