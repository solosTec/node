# 
#	reset 
#
set (broker-wmbus_node)

set (broker-wmbus_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (broker-wmbus_h
    include/controller.h
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


source_group("broker-wmbus-assets" FILES ${broker-wmbus_assets})


set (broker-wmbus_node
  ${broker-wmbus_cpp}
  ${broker-wmbus_h}
  ${broker-wmbus_assets}
)

