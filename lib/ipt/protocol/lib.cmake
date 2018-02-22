# top level files
set (ipt_protocol_lib)

set (ipt_protocol_cpp

	lib/ipt/protocol/src/codes.cpp
	lib/ipt/protocol/src/header.cpp
	lib/ipt/protocol/src/response.cpp
	lib/ipt/protocol/src/scramble_key.cpp
	lib/ipt/protocol/src/scramble_key_format.cpp
	lib/ipt/protocol/src/parser.cpp
	lib/ipt/protocol/src/serializer.cpp
)

set (ipt_protocol_h
	src/main/include/smf/ipt/defs.h
	src/main/include/smf/ipt/codes.h
	src/main/include/smf/ipt/header.h
	src/main/include/smf/ipt/response.hpp
	src/main/include/smf/ipt/scramble_key.h
	src/main/include/smf/ipt/scramble_key_format.h
	src/main/include/smf/ipt/scramble_key_io.hpp
	src/main/include/smf/ipt/parser.h
	src/main/include/smf/ipt/serializer.h
)


#source_group("shared" FILES ${ipt_shared})

# define the main program
set (ipt_protocol_lib
  ${ipt_protocol_cpp}
  ${ipt_protocol_h}
#  ${ipt_shared}
)

