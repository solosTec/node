# 
#	reset 
#
set (setup_node)

set (setup_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (setup_h
    include/controller.h
)

if(WIN32)
    set(setup_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(setup_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()


source_group("setup-assets" FILES ${setup_assets})


set (setup_node
  ${setup_cpp}
  ${setup_h}
  ${setup_assets}
)

