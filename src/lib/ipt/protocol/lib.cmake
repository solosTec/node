# 
#	reset 
#
set (ipt_lib)

set (ipt_cpp
    src/lib/ipt/protocol/src/scramble_key.cpp
    src/lib/ipt/protocol/src/scramble_key_format.cpp
)
    
set (ipt_h
    include/smf.h
    include/smf/ipt.h    
    include/smf/ipt/scramble_key.h
    include/smf/ipt/scramble_key_format.h
    include/smf/ipt/scramble_key_io.hpp
)


# define the ipt lib
set (ipt_lib
  ${ipt_cpp}
  ${ipt_h}
)

