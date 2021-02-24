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
    include/tasks/lmn.h
    include/tasks/gpio.h
    include/tasks/broker.h
    src/tasks/bridge.cpp
    src/tasks/lmn.cpp
    src/tasks/gpio.cpp
    src/tasks/broker.cpp
)

set (segw_config
    include/config/cfg_ipt.h
    include/config/cfg_lmn.h
    include/config/cfg_broker.h
    include/config/cfg_gpio.h
    src/config/cfg_ipt.cpp
    src/config/cfg_lmn.cpp
    src/config/cfg_broker.cpp
    src/config/cfg_gpio.cpp
)


source_group("segw-assets" FILES ${segw_assets})
source_group("tasks" FILES ${segw_tasks})
source_group("config" FILES ${segw_config})


set (segw_node
  ${segw_cpp}
  ${segw_h}
  ${segw_assets}
  ${segw_tasks}
  ${segw_config}
)

