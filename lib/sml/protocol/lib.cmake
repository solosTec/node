# top level files
set (sml_protocol_lib)

set (sml_protocol_cpp

	lib/sml/protocol/src/crc16.cpp
	lib/sml/protocol/src/units.cpp
	lib/sml/protocol/src/intrinsics/obis.cpp

)

set (sml_protocol_h
	src/main/include/smf/sml/defs.h
	src/main/include/smf/sml/crc16.h
	src/main/include/smf/sml/units.h
	src/main/include/smf/sml/intrinsics/obis.h
)


# define the main program
set (sml_protocol_lib
  ${sml_protocol_cpp}
  ${sml_protocol_h}
)

