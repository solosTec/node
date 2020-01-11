# top level files
set (info_lib)

set (segw_info_cpp

	nodes/print_build_info.cpp
	nodes/print_version_info.cpp
	nodes/set_start_options.cpp
	nodes/show_ip_address.cpp
	nodes/show_fs_drives.cpp
)

set (segw_info_h
	
	nodes/print_build_info.h
	nodes/print_version_info.h
	nodes/set_start_options.h
	nodes/show_ip_address.h
	nodes/show_fs_drives.h
)

if (UNIX)
	list(APPEND segw_info_h src/main/include/smf/shared/write_pid.h)
	list(APPEND segw_info_cpp nodes/shared/sys/write_pid.cpp)
endif(UNIX)


# define the main program
set (info_lib
  ${segw_info_cpp}
  ${segw_info_h}
)

