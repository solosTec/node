# 
#	reset 
#
set (segw_node)

set (segw_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (segw_h
    include/controller.h
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


source_group("segw-assets" FILES ${segw_assets})



set (segw_node
  ${segw_cpp}
  ${segw_h}
  ${segw_assets}
)

