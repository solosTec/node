# 
#	reset 
#
set (modem_lib)

set (modem_cpp
    src/lib/modem/protocol/src/parser.cpp
    src/lib/modem/protocol/src/serializer.cpp
)
    
set (modem_h
    include/smf/modem/parser.h
    include/smf/modem/serializer.h
)


# define the modem lib
set (modem_lib
  ${modem_cpp}
  ${modem_h}
)

