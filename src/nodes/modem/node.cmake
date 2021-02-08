# 
#	reset 
#
set (modem_node)

set (modem_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (modem_h
    include/controller.h
)


set (modem_node
  ${modem_cpp}
  ${modem_h}
)

