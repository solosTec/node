# top level files
set (ipt_bus_lib)

set (ipt_bus_cpp

	src/bus_interface.cpp
	src/bus.cpp
	src/generator.cpp
	src/config.cpp
)

set (ipt_bus_h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/defs.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/bus_interface.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/bus.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/generator.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/config.h
)

if(${PROJECT_NAME}_SSL_SUPPORT)
	list(APPEND ipt_bus_cpp src/bus_tls.cpp)
	list(APPEND ipt_bus_h ${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/bus_tls.h)
endif()

set (ipt_bus_tasks
	src/tasks/open_connection.h
	src/tasks/open_connection.cpp
	src/tasks/close_connection.h
	src/tasks/close_connection.cpp
	src/tasks/register_target.h
	src/tasks/register_target.cpp
	src/tasks/open_channel.h
	src/tasks/open_channel.cpp
	src/tasks/transfer_data.h
	src/tasks/transfer_data.cpp
	src/tasks/watchdog.h
	src/tasks/watchdog.cpp
)

source_group("tasks" FILES ${ipt_bus_tasks})

# define the main program
set (ipt_bus_lib
  ${ipt_bus_cpp}
  ${ipt_bus_h}
  ${ipt_bus_tasks}
)

