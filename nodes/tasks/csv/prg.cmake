# top level files
set (task_csv)
set (task_csv_root nodes/tasks/csv)

set (task_csv_cpp

	${task_csv_root}/src/main.cpp	
	${task_csv_root}/src/controller.cpp
)

set (task_csv_h

	${task_csv_root}/src/controller.h
)

set (task_csv_schemes

	src/main/include/smf/shared/db_meta.h
	nodes/shared/db/db_meta.cpp
)

set (task_csv_tasks
	${task_csv_root}/src/tasks/cluster.h
	${task_csv_root}/src/tasks/cluster.cpp
	${task_csv_root}/src/tasks/storage_db.h
	${task_csv_root}/src/tasks/storage_db.cpp
	${task_csv_root}/src/tasks/profile_15_min.h
	${task_csv_root}/src/tasks/profile_15_min.cpp
	${task_csv_root}/src/tasks/profile_60_min.h
	${task_csv_root}/src/tasks/profile_60_min.cpp
	${task_csv_root}/src/tasks/profile_24_h.h
	${task_csv_root}/src/tasks/profile_24_h.cpp
)
	
if(WIN32)

	set (task_csv_service
		${task_csv_root}/templates/csv_create_service.cmd.in
		${task_csv_root}/templates/csv_delete_service.cmd.in
		${task_csv_root}/templates/csv_restart_service.cmd.in
		${task_csv_root}/templates/csv.windows.cgf.in
	)
 
 	set (task_csv_manifest
		${task_csv_root}/templates/csv.exe.manifest
	)

 	set (task_csv_res
		${CMAKE_CURRENT_BINARY_DIR}/csv.rc 
		src/main/resources/logo.ico
	)

else()

	set (task_csv_service
		${task_csv_root}/templates/csv.service.in
		${task_csv_root}/templates/csv.linux.cgf.in
	)

endif()

set (task_csv_shared

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
