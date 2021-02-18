# 
#	reset 
#
set (imega_node)

set (imega_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (imega_h
    include/controller.h
)

if(WIN32)
    set(imega_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(imega_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()


source_group("imega-assets" FILES ${imega_assets})

set (imega_node
  ${imega_cpp}
  ${imega_h}
  ${imega_assets}
)

