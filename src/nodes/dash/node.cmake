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
)
    
set (dash_h
    include/controller.h
    include/http_server.h
    include/db.h
    include/notifier.h
    include/upload.h
)

if(WIN32)
    set(dash_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
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

