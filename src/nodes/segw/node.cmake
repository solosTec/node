# 
#	reset 
#
set (segw_node)

set (segw_cpp
    src/main.cpp
    src/controller.cpp
    src/storage.cpp
    src/storage_functions.cpp
    src/cfg.cpp
)
    
set (segw_h
    include/controller.h
    include/storage.h
    include/storage_functions.h
    include/cfg.h
)

if(WIN32)
    set(segw_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(segw_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()

set (segw_tasks
    include/tasks/bridge.h
    src/tasks/bridge.cpp
)


source_group("segw-assets" FILES ${segw_assets})
source_group("tasks" FILES ${segw_tasks})


set (segw_node
  ${segw_cpp}
  ${segw_h}
  ${segw_assets}
  ${segw_tasks}
)

