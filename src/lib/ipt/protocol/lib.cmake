# 
#	reset 
#
set (ipt_lib)

set (ipt_cpp
    src/lib/ipt/protocol/src/scramble_key.cpp
    src/lib/ipt/protocol/src/scramble_key_format.cpp
    src/lib/ipt/protocol/src/codes.cpp
    src/lib/ipt/protocol/src/header.cpp
    src/lib/ipt/protocol/src/response.cpp
    src/lib/ipt/protocol/src/parser.cpp
)
    
set (ipt_h
    include/smf.h
    include/smf/ipt.h    
    include/smf/ipt/scramble_key.h
    include/smf/ipt/scramble_key_format.h
    include/smf/ipt/scramble_key_io.hpp
    include/smf/ipt/codes.h
    include/smf/ipt/header.h
    include/smf/ipt/response.hpp
    include/smf/ipt/scrambler.hpp
    include/smf/ipt/parser.h
)


# define the ipt lib
set (ipt_lib
  ${ipt_cpp}
  ${ipt_h}
)

