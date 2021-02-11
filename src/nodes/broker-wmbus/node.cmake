# 
#	reset 
#
set (broker-wmbus_node)

set (broker-wmbus_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (broker-wmbus_h
    include/controller.h
)


set (broker-wmbus_node
  ${broker-wmbus_cpp}
  ${broker-wmbus_h}
)

