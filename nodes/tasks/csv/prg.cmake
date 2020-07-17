# top level files
set (task_csv)

set (task_csv_cpp

	tasks/csv/src/main.cpp	
	tasks/csv/src/controller.cpp
)

set (task_csv_h

	tasks/csv/src/controller.h
)

set (task_csv_schemes

	src/main/include/smf/shared/db_meta.h
	nodes/shared/db/db_meta.cpp
)

set (task_csv_tasks
	tasks/csv/src/tasks/cluster.h
	tasks/csv/src/tasks/cluster.cpp
	tasks/csv/src/tasks/storage_db.h
	tasks/csv/src/tasks/storage_db.cpp
	tasks/csv/src/tasks/profile_15_min.h
	tasks/csv/src/tasks/profile_15_min.cpp
	tasks/csv/src/tasks/profile_60_min.h
	tasks/csv/src/tasks/profile_60_min.cpp
	tasks/csv/src/tasks/profile_24_h.h
	tasks/csv/src/tasks/profile_24_h.cpp
)
	
if(WIN32)

	set (task_csv_service
		tasks/csv/templates/csv_create_service.cmd.in
		tasks/csv/templates/csv_delete_service.cmd.in
		tasks/csv/templates/csv_restart_service.cmd.in
		tasks/csv/templates/csv.windows.cgf.in
	)
 
 	set (task_csv_manifest
		tasks/csv/templates/csv.exe.manifest
	)

 	set (task_csv_res
		${CMAKE_CURRENT_BINARY_DIR}/csv.rc 
		src/main/resources/logo.ico
	)

else()

	set (task_csv_service
		tasks/csv/templates/csv.service.in
		tasks/csv/templates/csv.linux.cgf.in
	)

endif()

set (task_csv_shared

	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	tasks/print_version_info.h
	tasks/set_start_options.h
	nodes/show_ip_address.h

	nodes/print_build_info.cpp
	tasks/print_version_info.cpp
	tasks/set_start_options.cpp
	nodes/show_ip_address.cpp

	src/main/include/smf/shared/ctl.h
	nodes/shared/sys/ctl.cpp
)

if (UNIX)
	list(APPEND task_csv_shared src/main/include/smf/shared/write_pid.h)
	list(APPEND task_csv_shared nodes/shared/sys/write_pid.cpp)
endif(UNIX)

source_group("tasks" FILES ${task_csv_tasks})
source_group("service" FILES ${task_csv_service})
source_group("shared" FILES ${task_csv_shared})
source_group("schemes" FILES ${task_csv_schemes})


# define the main program
set (task_csv
  ${task_csv_cpp}
  ${task_csv_h}
  ${task_csv_shared}
  ${task_csv_tasks}
  ${task_csv_service}
  ${task_csv_schemes}
)

if(WIN32)
	source_group("manifest" FILES ${task_csv_manifest})
	list(APPEND task_csv ${task_csv_manifest})
	source_group("resources" FILES ${task_csv_res})
	list(APPEND task_csv ${task_csv_res})
endif()
