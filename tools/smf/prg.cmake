# top level files
set (tool_smf)

set (tool_smf_cpp

	src/main.cpp	
	src/controller.cpp
	src/console.cpp
	src/cli.cpp
)

set (tool_smf_h

	src/controller.h	
	src/console.h	
	src/cli.h	

)

set (tool_smf_shared

	${CMAKE_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h
	${CMAKE_SOURCE_DIR}/tools/set_start_options.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h

	${CMAKE_SOURCE_DIR}/lib/sml/exporter/src/abl_sml_exporter.cpp
)

if (UNIX)
	list(APPEND tool_smf_shared ${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/write_pid.h)
	list(APPEND tool_smf_shared ${CMAKE_SOURCE_DIR}/nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (tool_smf_plugin

	src/plugins/convert.h
	src/plugins/convert_csv_abl.h
	src/plugins/convert.cpp
	src/plugins/tracking.h
	src/plugins/tracking.cpp
	src/plugins/cleanup-v4.h
	src/plugins/cleanup-v4.cpp
	src/plugins/send.h
	src/plugins/send.cpp
	src/plugins/join.h
	src/plugins/join.cpp
)

set (tool_smf_tasks

	src/plugins/tasks/cluster.h
	src/plugins/tasks/cluster.cpp
)
source_group("shared" FILES ${tool_smf_shared})
source_group("plugin" FILES ${tool_smf_plugin})
source_group("tasks" FILES ${tool_smf_tasks})

# define the main program
set (tool_smf
  ${tool_smf_cpp}
  ${tool_smf_h}
  ${tool_smf_shared}
  ${tool_smf_plugin}
  ${tool_smf_tasks}
)

