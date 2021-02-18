# 
#	reset 
#
set (dash_node)

set (dash_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (dash_h
    include/controller.h
)

if(WIN32)
    set(dash_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(dash_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()


source_group("dash-assets" FILES ${dash_assets})


set (dash_node
  ${dash_cpp}
  ${dash_h}
  ${dash_assets}
)

