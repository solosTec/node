# 
#	reset 
#
set (main_node)

set (main_cpp
    src/main.cpp
    src/controller.cpp
    src/session.cpp
    src/db.cpp
)
    
set (main_h
    include/controller.h
    include/session.h
    include/db.h
)

if(WIN32)
    set(main_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
		templates/rc.in
    )
	set (main_res
		${PROJECT_BINARY_DIR}/main.rc 
		${CMAKE_SOURCE_DIR}/assets/logo.ico
#		templates/master.exe.manifest
	)

else()
    set(main_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()

set (main_tasks
  include/tasks/ping.h 
  src/tasks/ping.cpp
)

source_group("main-assets" FILES ${main_assets})
source_group("tasks" FILES ${main_tasks})


set (main_node
  ${main_cpp}
  ${main_h}
  ${main_assets}
  ${main_tasks}
)

if(WIN32)
	source_group("resources" FILES ${main_res})
	list(APPEND main_node ${main_res})
endif()