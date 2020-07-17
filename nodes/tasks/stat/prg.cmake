# top level files
set (task_stat)

set (task_stat_cpp

	tasks/stat/src/main.cpp	
	tasks/stat/src/controller.cpp
)

set (task_stat_h

	tasks/stat/src/controller.h
)

set (task_stat_schemes

	src/main/include/smf/shared/db_meta.h
	nodes/shared/db/db_meta.cpp
)

set (task_stat_shared
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	tasks/print_version_info.h
	tasks/set_start_options.h
	nodes/show_ip_address.h

	src/main/include/smf/shared/ctl.h
)

if (UNIX)
	list(APPEND task_stat_shared src/main/include/smf/shared/write_pid.h)
	list(APPEND task_stat_shared nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (task_stat_tasks
	tasks/stat/src/tasks/cluster.h
	tasks/stat/src/tasks/cluster.cpp
)
	
if(WIN32)

	set (task_stat_service
		tasks/stat/templates/stat_create_service.cmd.in
		tasks/stat/templates/stat_delete_service.cmd.in
		tasks/stat/templates/stat_restart_service.cmd.in
		tasks/stat/templates/stat.windows.cgf.in
	)
 
 	set (task_stat_manifest
		tasks/stat/templates/stat.exe.manifest
	)

 	set (task_stat_res
		${CMAKE_CURRENT_BINARY_DIR}/stat.rc 
		src/main/resources/logo.ico
	)

else()

	set (task_stat_service
		tasks/stat/templates/stat.service.in
		tasks/stat/templates/stat.linux.cgf.in
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
