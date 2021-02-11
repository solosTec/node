# 
#	reset 
#
set (broker-iec_node)

set (broker-iec_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (broker-iec_h
    include/controller.h
)

if(WIN32)
    set(broker-iec_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(broker-iec_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()


source_group("broker-iec-assets" FILES ${broker-iec_assets})

set (broker-iec_node
  ${broker-iec_cpp}
  ${broker-iec_h}
  ${broker-iec_assets}
)

