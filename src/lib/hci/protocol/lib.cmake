# 
#	reset 
#
set (hci_lib)

set (hci_cpp
    src/lib/hci/protocol/src/parser.cpp
    src/lib/hci/protocol/src/msg.cpp
)
    
set (hci_h
    include/smf/hci.h    
    include/smf/hci/parser.h
    include/smf/hci/msg.h
)


# define the hci lib
set (hci_lib
  ${hci_cpp}
  ${hci_h}
)

