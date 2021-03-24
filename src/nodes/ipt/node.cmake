# 
#	reset 
#
set (ipt_node)

set (ipt_cpp
    src/main.cpp
    src/controller.cpp
    src/ipt_server.cpp
    src/ipt_session.cpp
)
    
set (ipt_h
    include/controller.h
    include/ipt_server.h
    include/ipt_session.h
)

set (ipt_tasks
  include/tasks/cluster.h
  src/tasks/cluster.cpp
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

