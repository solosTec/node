# 
#	reset 
#
set (mqtt_node)

set (mqtt_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (mqtt_h
    include/controller.h
)

if(WIN32)
    set(mqtt_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(mqtt_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()


source_group("mqtt-assets" FILES ${mqtt_assets})


set (mqtt_node
  ${mqtt_cpp}
  ${mqtt_h}
  ${mqtt_assets}
)

