# 
#	reset 
#
set (iec_lib)

set (iec_cpp
    src/lib/iec/protocol/src/defs.cpp
    src/lib/iec/protocol/src/parser.cpp
    src/lib/iec/protocol/src/scanner.cpp
)
    
set (iec_h
    include/smf/iec.h    
    include/smf/iec/defs.h
    include/smf/iec/parser.h
    include/smf/iec/scanner.h
)


# define the iec lib
set (iec_lib
  ${iec_cpp}
  ${iec_h}
)

