# top level files
set (iec_protocol_lib)

set (iec_protocol_cpp

	src/parser.cpp
#	src/serializer.cpp
)

set (iec_protocol_h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/iec/defs.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/iec/parser.h
)

# define the main program
set (iec_protocol_lib
  ${iec_protocol_cpp}
  ${iec_protocol_h}
)

