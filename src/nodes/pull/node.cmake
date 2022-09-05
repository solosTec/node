# 
#	reset 
#
set (pull_node)

set (pull_cpp
    src/main.cpp
    src/controller.cpp
    src/tasks/network.cpp
    src/tasks/target.cpp
)
    
set (pull_h
    include/controller.h
    include/tasks/network.h
    include/tasks/target.h
)

if(WIN32)
    set(pull_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
		templates/rc.in
    )

	set (pull_res
		${PROJECT_BINARY_DIR}/pull.rc 
		${CMAKE_SOURCE_DIR}/assets/logo.ico
#		templates/master.exe.manifest
	)

else()
    set(pull_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()

set (pull_tasks
)


source_group("pull-assets" FILES ${pull_assets})
source_group("tasks" FILES ${pull_tasks})


set (pull_node
  ${pull_cpp}
  ${pull_h}
  ${pull_assets}
  ${pull_tasks}
)

if(WIN32)
	source_group("resources" FILES ${pull_res})
	list(APPEND pull_node ${pull_res})
endif()