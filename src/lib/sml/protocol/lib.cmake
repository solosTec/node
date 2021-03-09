# 
#	reset 
#
set (sml_lib)

set (sml_cpp
    src/lib/sml/protocol/src/sml.cpp
    src/lib/sml/protocol/src/tokenizer.cpp
    src/lib/sml/protocol/src/parser.cpp
    src/lib/sml/protocol/src/crc16.cpp
    src/lib/sml/protocol/src/status.cpp
    src/lib/sml/protocol/src/event.cpp
)
    
set (sml_h
    include/smf/sml.h
    include/smf/sml/tokenizer.h    
    include/smf/sml/parser.h
    include/smf/sml/crc16.h
    include/smf/sml/status.h
    include/smf/sml/event.h
)


# define the sml lib
set (sml_lib
  ${sml_cpp}
  ${sml_h}
)

