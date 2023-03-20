# 
#	reset 
#
set (ipt_node)

set (ipt_cpp
    src/main.cpp
    src/controller.cpp
    src/session.cpp
    src/proxy.cpp
)
    
set (ipt_h
    include/controller.h
    include/session.h
    include/proxy.h
)

set (ipt_tasks
    include/tasks/cluster.h
#    include/tasks/gatekeeper.h
    src/tasks/cluster.cpp
#    src/tasks/gatekeeper.cpp
)

if(WIN32)
    set(ipt_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
		templates/rc.in
    )
	set (ipt_res
		${PROJECT_BINARY_DIR}/ipt.rc 
		${CMAKE_SOURCE_DIR}/assets/logo.ico
#		templates/master.exe.manifest
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

if(WIN32)
	source_group("resources" FILES ${ipt_res})
	list(APPEND ipt_node ${ipt_res})
endif()