# 
#	reset 
#
set (imega_lib)

set (imega_cpp
    src/lib/imega/protocol/src/parser.cpp
    src/lib/imega/protocol/src/serializer.cpp
    src/lib/imega/protocol/src/policy.cpp
)
    
set (imega_h
    include/smf/imega.h
    include/smf/imega/parser.h
    include/smf/imega/serializer.h
)


# define the imega lib
set (imega_lib
  ${imega_cpp}
  ${imega_h}
)

