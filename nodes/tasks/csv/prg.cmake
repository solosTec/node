# top level files
set (task_csv)

set (task_csv_cpp

	src/main.cpp	
	src/controller.cpp
)

set (task_csv_h

	src/controller.h
)

set (task_csv_schemes

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/db_meta.h
	${CMAKE_SOURCE_DIR}/nodes/shared/db/db_meta.cpp
)

set (task_csv_tasks
	src/tasks/cluster.h
	src/tasks/cluster.cpp
	src/tasks/storage_db.h
	src/tasks/storage_db.cpp
	src/tasks/profile_15_min.h
	src/tasks/profile_15_min.cpp
	src/tasks/profile_60_min.h
	src/tasks/profile_60_min.cpp
	src/tasks/profile_24_h.h
	src/tasks/profile_24_h.cpp
)
	
if(WIN32)

	set (task_csv_service
		templates/csv_create_service.cmd.in
		templates/csv_delete_service.cmd.in
		templates/csv_restart_service.cmd.in
		templates/csv.windows.cgf.in
	)
 
 	set (task_csv_manifest
		templates/csv.exe.manifest
	)

 	set (task_csv_res
		${CMAKE_BINARY_DIR}/csv.rc 
		${CMAKE_SOURCE_DIR}/src/main/resources/logo.ico
	)

else()

	set (task_csv_service
		templates/csv.service.in
		templates/csv.linux.cgf.in
	)

endif()

set (task_csv_shared

	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/tasks/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/tasks/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h
#	${CMAKE_SOURCE_DIR}/nodes/shared/sys/ctl.cpp
)

if (UNIX)
	list(APPEND task_csv_shared ${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/write_pid.h)
	list(APPEND task_csv_shared ${CMAKE_SOURCE_DIR}/nodes/shared/sys/write_pid.cpp)
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
