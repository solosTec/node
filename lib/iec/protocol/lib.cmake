# top level files
set (iec_protocol_lib)

set (iec_protocol_cpp

	lib/iec/protocol/src/parser.cpp
#	lib/iec/protocol/src/serializer.cpp
)

set (iec_protocol_h
	src/main/include/smf/iec/defs.h
	src/main/include/smf/iec/parser.h
#	src/main/include/smf/iec/serializer.h
)

# define the main program
set (iec_protocol_lib
  ${iec_protocol_cpp}
  ${iec_protocol_h}
)

