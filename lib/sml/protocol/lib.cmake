# top level files
set (sml_protocol_lib)

set (sml_protocol_cpp

	lib/sml/protocol/src/crc16.cpp
	lib/sml/protocol/src/units.cpp
	lib/sml/protocol/src/scaler.cpp
	lib/sml/protocol/src/value.cpp
	lib/sml/protocol/src/parser.cpp
	lib/sml/protocol/src/serializer.cpp
	lib/sml/protocol/src/writer.hpp
	lib/sml/protocol/src/writer.cpp
	lib/sml/protocol/src/intrinsics/obis.cpp
	lib/sml/protocol/src/mbus_defs.cpp
	lib/sml/protocol/src/obis_db.cpp
	lib/sml/protocol/src/obis_io.cpp
	lib/sml/protocol/src/srv_id_io.cpp
)

set (sml_protocol_h
	src/main/include/smf/sml/defs.h
	src/main/include/smf/sml/crc16.h
	src/main/include/smf/sml/units.h
	src/main/include/smf/sml/scaler.h
	src/main/include/smf/sml/protocol/parser.h
	src/main/include/smf/sml/protocol/serializer.h
	src/main/include/smf/sml/intrinsics/obis.h
	src/main/include/smf/sml/protocol/value.hpp
	src/main/include/smf/sml/mbus/defs.h
	src/main/include/smf/sml/obis_db.h
	src/main/include/smf/sml/obis_io.h
	src/main/include/smf/sml/srv_id_io.h
)


# define the main program
set (sml_protocol_lib
  ${sml_protocol_cpp}
  ${sml_protocol_h}
)

