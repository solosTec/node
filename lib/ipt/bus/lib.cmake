# top level files
set (ipt_bus_lib)

set (ipt_bus_cpp

	lib/ipt/bus/src/bus.cpp
	lib/ipt/bus/src/generator.cpp
	lib/ipt/bus/src/config.cpp
)

set (ipt_bus_h
	src/main/include/smf/ipt/defs.h
	src/main/include/smf/ipt/bus.h
	src/main/include/smf/ipt/generator.h
	src/main/include/smf/ipt/config.h
)

set (ipt_bus_tasks
	lib/ipt/bus/src/tasks/open_connection.h
	lib/ipt/bus/src/tasks/open_connection.cpp
	lib/ipt/bus/src/tasks/close_connection.h
	lib/ipt/bus/src/tasks/close_connection.cpp
	lib/ipt/bus/src/tasks/register_target.h
	lib/ipt/bus/src/tasks/register_target.cpp
	lib/ipt/bus/src/tasks/watchdog.h
	lib/ipt/bus/src/tasks/watchdog.cpp
)

source_group("tasks" FILES ${ipt_bus_tasks})

# define the main program
set (ipt_bus_lib
  ${ipt_bus_cpp}
  ${ipt_bus_h}
  ${ipt_bus_tasks}
)

