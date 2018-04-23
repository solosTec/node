# top level files
set (imega_protocol_lib)

set (imega_protocol_cpp

#	lib/imega/protocol/src/codes.cpp
#	lib/imega/protocol/src/header.cpp
#	lib/imega/protocol/src/response.cpp
	lib/imega/protocol/src/parser.cpp
	lib/imega/protocol/src/serializer.cpp
)

set (imega_protocol_h
#	src/main/include/smf/imega/defs.h
#	src/main/include/smf/imega/codes.h
#	src/main/include/smf/imega/header.h
#	src/main/include/smf/imega/response.hpp
	src/main/include/smf/imega/parser.h
	src/main/include/smf/imega/serializer.h
)


#source_group("shared" FILES ${imega_shared})

# define the main program
set (imega_protocol_lib
  ${imega_protocol_cpp}
  ${imega_protocol_h}
)

