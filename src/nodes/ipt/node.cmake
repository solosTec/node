# 
#	reset 
#
set (ipt_node)

set (ipt_cpp
    src/main.cpp
    src/controller.cpp
    src/server.cpp
    src/session.cpp
)
    
set (ipt_h
    include/controller.h
    include/server.h
    include/session.h
)

set (ipt_tasks
    include/tasks/cluster.h
    include/tasks/gatekeeper.h
    include/tasks/proxy.h
    src/tasks/cluster.cpp
    src/tasks/gatekeeper.cpp
    src/tasks/proxy.cpp
)

if(WIN32)
    set(ipt_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(ipt_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()


source_group("ipt-assets" FILES ${ipt_assets})
source_group("tasks" FILES ${ipt_tasks})

set (ipt_node
  ${ipt_cpp}
  ${ipt_h}
  ${ipt_assets}
  ${ipt_tasks}
)

