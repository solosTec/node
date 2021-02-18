# 
#	reset 
#
set (modem_node)

set (modem_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (modem_h
    include/controller.h
)

if(WIN32)
    set(modem_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(modem_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()


source_group("modem-assets" FILES ${modem_assets})


set (modem_node
  ${modem_cpp}
  ${modem_h}
  ${modem_assets}
)

