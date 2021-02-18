# 
#	reset 
#
set (lora_node)

set (lora_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (lora_h
    include/controller.h
)

if(WIN32)
    set(lora_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(lora_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()


source_group("LoRa-assets" FILES ${lora_assets})


set (lora_node
  ${lora_cpp}
  ${lora_h}
  ${lora_assets}
)

