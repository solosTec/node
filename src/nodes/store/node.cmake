# 
#	reset 
#
set (store_node)

set (store_cpp
     src/main.cpp
    src/controller.cpp
)
    
set (store_h
    include/controller.h
)

if(WIN32)
    set(store_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(store_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()


source_group("store-assets" FILES ${store_assets})


set (store_node
  ${store_cpp}
  ${store_h}
  ${store_assets}
)

