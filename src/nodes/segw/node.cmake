# 
#	reset 
#
set (segw_node)

set (segw_cpp
    src/main.cpp
    src/controller.cpp
    src/storage.cpp
    src/storage_functions.cpp
    src/distributor.cpp
    src/router.cpp
)
    
set (segw_h
    include/controller.h
    include/storage.h
    include/storage_functions.h
    include/distributor.h
    include/router.h
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
    include/tasks/CP210x.h
    include/tasks/persistence.h
    include/tasks/filter.h
    include/tasks/redirector.h
    include/tasks/nms.h
    src/tasks/bridge.cpp
    src/tasks/lmn.cpp
    src/tasks/gpio.cpp
    src/tasks/broker.cpp
    src/tasks/CP210x.cpp
    src/tasks/persistence.cpp
    src/tasks/filter.cpp
    src/tasks/redirector.cpp
    src/tasks/nms.cpp
)

set (segw_config
    include/cfg.h
    include/config/cfg_ipt.h
    include/config/cfg_lmn.h
    include/config/cfg_broker.h
    include/config/cfg_listener.h
    include/config/cfg_blocklist.h
    include/config/cfg_gpio.h
    include/config/cfg_nms.h
    include/config/cfg_sml.h
    include/config/cfg_vmeter.h
    include/config/cfg_hardware.h
    src/cfg.cpp
    src/config/cfg_ipt.cpp
    src/config/cfg_lmn.cpp
    src/config/cfg_broker.cpp
    src/config/cfg_listener.cpp
    src/config/cfg_blocklist.cpp
    src/config/cfg_gpio.cpp
    src/config/cfg_nms.cpp
    src/config/cfg_sml.cpp
    src/config/cfg_vmeter.cpp
    src/config/cfg_hardware.cpp
)

set (segw_sml
    include/sml/server.h
    include/sml/session.h
    src/sml/server.cpp
    src/sml/session.cpp
)

set (segw_nms
    include/nms/session.h
    include/nms/reader.h
    src/nms/session.cpp
    src/nms/reader.cpp
)

set (segw_redirector
    include/redirector/server.h
    include/redirector/session.h
    src/redirector/server.cpp
    src/redirector/session.cpp
)


source_group("segw-assets" FILES ${segw_assets})
source_group("tasks" FILES ${segw_tasks})
source_group("config" FILES ${segw_config})
source_group("sml" FILES ${segw_sml})
source_group("nms" FILES ${segw_nms})
source_group("redirector" FILES ${segw_redirector})


set (segw_node
  ${segw_cpp}
  ${segw_h}
  ${segw_assets}
  ${segw_tasks}
  ${segw_config}
  ${segw_sml}
  ${segw_nms}
  ${segw_redirector}
)

