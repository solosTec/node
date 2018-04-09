# top level files
set (modem_protocol_lib)

set (modem_protocol_cpp

#	lib/modem/protocol/src/codes.cpp
#	lib/modem/protocol/src/header.cpp
#	lib/modem/protocol/src/response.cpp
	lib/modem/protocol/src/parser.cpp
	lib/modem/protocol/src/serializer.cpp
)

set (modem_protocol_h
#	src/main/include/smf/modem/defs.h
#	src/main/include/smf/modem/codes.h
#	src/main/include/smf/modem/header.h
#	src/main/include/smf/modem/response.hpp
	src/main/include/smf/modem/parser.h
	src/main/include/smf/modem/serializer.h
)


#source_group("shared" FILES ${modem_shared})

# define the main program
set (modem_protocol_lib
  ${modem_protocol_cpp}
  ${modem_protocol_h}
)

