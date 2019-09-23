# top level files
set (tool_smf)

set (tool_smf_cpp

	tools/smf/src/main.cpp	
	tools/smf/src/controller.cpp
	tools/smf/src/console.cpp
	tools/smf/src/cli.cpp
)

set (tool_smf_h

	tools/smf/src/controller.h	
	tools/smf/src/console.h	
	tools/smf/src/cli.h	

)

set (tool_smf_shared

	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_project_info.h

	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/show_ip_address.h
	tools/set_start_options.h

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/show_ip_address.cpp
	tools/set_start_options.cpp

	src/main/include/smf/shared/ctl.h
	nodes/shared/sys/ctl.cpp

	lib/sml/exporter/src/abl_sml_exporter.cpp
)

if (UNIX)
	list(APPEND tool_smf_shared src/main/include/smf/shared/write_pid.h)
	list(APPEND tool_smf_shared nodes/shared/sys/write_pid.cpp)
endif(UNIX)

set (tool_smf_plugin

	tools/smf/src/convert.h
	tools/smf/src/convert.cpp

)

source_group("shared" FILES ${tool_smf_shared})
source_group("plugin" FILES ${tool_smf_plugin})

# define the main program
set (tool_smf
  ${tool_smf_cpp}
  ${tool_smf_h}
  ${tool_smf_shared}
  ${tool_smf_plugin}
)

