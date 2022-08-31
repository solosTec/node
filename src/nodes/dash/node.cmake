# 
#	reset 
#
set (dash_node)

set (dash_cpp
    src/main.cpp
    src/controller.cpp
    src/http_server.cpp
    src/db.cpp
    src/notifier.cpp
    src/upload.cpp
    src/download.cpp
)
    
set (dash_h
    include/controller.h
    include/http_server.h
    include/db.h
    include/notifier.h
    include/upload.h
    include/download.h
)

if(WIN32)
    set(dash_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
		templates/rc.in
    )
	set (dash_res
		${PROJECT_BINARY_DIR}/dash.rc 
		${CMAKE_SOURCE_DIR}/assets/logo.ico
#		templates/master.exe.manifest
	)

else()
    set(dash_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()

set (dash_tasks
  include/tasks/cluster.h
  src/tasks/cluster.cpp
)


source_group("dash-assets" FILES ${dash_assets})
source_group("tasks" FILES ${dash_tasks})


set (dash_node
  ${dash_cpp}
  ${dash_h}
  ${dash_assets}
  ${dash_tasks}
)

if(WIN32)
	source_group("resources" FILES ${dash_res})
	list(APPEND dash_node ${dash_res})
endif()