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


set (lora_node
  ${lora_cpp}
  ${lora_h}
)

