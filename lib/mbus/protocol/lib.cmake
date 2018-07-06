# top level files
set (mbus_protocol_lib)

set (mbus_protocol_cpp

#	lib/mbus/protocol/src/codes.cpp
#	lib/mbus/protocol/src/header.cpp
#	lib/mbus/protocol/src/response.cpp
	lib/mbus/protocol/src/parser.cpp
#	lib/mbus/protocol/src/serializer.cpp
)

set (mbus_protocol_h
	src/main/include/smf/mbus/defs.h
#	src/main/include/smf/mbus/codes.h
#	src/main/include/smf/mbus/header.h
#	src/main/include/smf/mbus/response.hpp
#	src/main/include/smf/mbus/scramble_key.h
	src/main/include/smf/mbus/parser.h
#	src/main/include/smf/mbus/serializer.h
)

# define the main program
set (mbus_protocol_lib
  ${mbus_protocol_cpp}
  ${mbus_protocol_h}
)

