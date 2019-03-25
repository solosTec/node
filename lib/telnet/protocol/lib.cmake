# top level files
set (telnet_protocol_lib)

set (telnet_protocol_cpp

	lib/telnet/protocol/src/parser.cpp
)

set (telnet_protocol_h
	src/main/include/smf/telnet/defs.h
	src/main/include/smf/telnet/parser.h
)


# define the main program
set (telnet_protocol_lib
  ${telnet_protocol_cpp}
  ${telnet_protocol_h}
)

