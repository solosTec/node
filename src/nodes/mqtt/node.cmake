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


set (mqtt_node
  ${mqtt_cpp}
  ${mqtt_h}
)

