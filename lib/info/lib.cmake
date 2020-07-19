# top level files
set (info_lib)

set (segw_info_cpp

	src/print_build_info.cpp
	src/print_version_info.cpp
	src/set_start_options.cpp
	src/show_ip_address.cpp
	src/show_fs_drives.cpp
	src/show_ports.cpp

	${CMAKE_SOURCE_DIR}/nodes/shared/sys/ctl.cpp

)

set (segw_info_h
	
	${CMAKE_SOURCE_DIR}/nodes/print_build_info.h
	${CMAKE_SOURCE_DIR}/nodes/print_version_info.h
	${CMAKE_SOURCE_DIR}/nodes/set_start_options.h
	${CMAKE_SOURCE_DIR}/nodes/show_ip_address.h
	${CMAKE_SOURCE_DIR}/nodes/show_fs_drives.h
	${CMAKE_SOURCE_DIR}/nodes/show_ports.h

	${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/ctl.h
)

if (UNIX)
	list(APPEND segw_info_h ${CMAKE_SOURCE_DIR}/src/main/include/smf/shared/write_pid.h)
	list(APPEND segw_info_cpp ${CMAKE_SOURCE_DIR}/lib/shared/sys/write_pid.cpp)
endif(UNIX)


# define the main program
set (info_lib
  ${segw_info_cpp}
  ${segw_info_h}
)

