# 
#	reset 
#
set (broker-iec_node)

set (broker-iec_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (broker-iec_h
    include/controller.h
)


set (broker-iec_node
  ${broker-iec_cpp}
  ${broker-iec_h}
)

