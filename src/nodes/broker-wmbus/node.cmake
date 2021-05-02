# 
#	reset 
#
set (broker-wmbus_node)

set (broker-wmbus_cpp
    src/main.cpp
    src/controller.cpp
    src/wmbus_server.cpp
    src/wmbus_session.cpp
    src/db.cpp
)
    
set (broker-wmbus_h
    include/controller.h
    include/wmbus_server.h
    include/wmbus_session.h
    include/db.h
)

if(WIN32)
    set(broker-wmbus_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(broker-wmbus_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()

set (broker-wmbus_tasks
    include/tasks/cluster.h
    include/tasks/push.h
    include/tasks/gatekeeper.h
    src/tasks/cluster.cpp
    src/tasks/push.cpp
    src/tasks/gatekeeper.cpp
)

source_group("broker-wmbus-assets" FILES ${broker-wmbus_assets})
source_group("tasks" FILES ${broker-wmbus_tasks})


set (broker-wmbus_node
  ${broker-wmbus_cpp}
  ${broker-wmbus_h}
  ${broker-wmbus_assets}
  ${broker-wmbus_tasks}
)

