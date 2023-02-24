# 
#	reset 
#
set (imega_node)

set (imega_cpp
    src/main.cpp
    src/controller.cpp
    src/session.cpp
)
    
set (imega_h
    include/controller.h
    include/session.h
)

set (imega_tasks
    include/tasks/cluster.h
    include/tasks/gatekeeper.h
    src/tasks/cluster.cpp
    src/tasks/gatekeeper.cpp
)

if(WIN32)
    set(imega_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(imega_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()


source_group("imega-assets" FILES ${imega_assets})
source_group("tasks" FILES ${modem_tasks})


set (imega_node
  ${imega_cpp}
  ${imega_h}
  ${imega_assets}
  ${imega_tasks}
)

