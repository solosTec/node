# top level files
set (sml_protocol_lib)

set (sml_protocol_cpp

	lib/sml/protocol/src/crc16.cpp
	lib/sml/protocol/src/units.cpp
	lib/sml/protocol/src/scaler.cpp
	lib/sml/protocol/src/writer.hpp
	lib/sml/protocol/src/writer.cpp
	lib/sml/protocol/src/reader.cpp
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
	src/main/include/smf/sml/intrinsics/obis.h
	src/main/include/smf/mbus/defs.h
	src/main/include/smf/sml/obis_db.h
	src/main/include/smf/sml/obis_io.h
	src/main/include/smf/sml/srv_id_io.h
)

set (sml_generator
	src/main/include/smf/sml/protocol/parser.h
	src/main/include/smf/sml/protocol/reader.h
	src/main/include/smf/sml/protocol/serializer.h
	src/main/include/smf/sml/protocol/message.h
	src/main/include/smf/sml/protocol/generator.h
	src/main/include/smf/sml/protocol/value.hpp

	lib/sml/protocol/src/parser.cpp
	lib/sml/protocol/src/serializer.cpp
	lib/sml/protocol/src/message.cpp
	lib/sml/protocol/src/generator.cpp
	lib/sml/protocol/src/value.cpp
)

set (sml_parser
	src/main/include/smf/sml/parser/obis_parser.h

	lib/sml/protocol/src/parser/obis_parser.cpp
	lib/sml/protocol/src/parser/obis_parser.hpp
)

source_group("generator" FILES ${sml_generator})
source_group("parser" FILES ${sml_parser})


# define the main program
set (sml_protocol_lib
  ${sml_protocol_cpp}
  ${sml_protocol_h}
  ${sml_generator}
  ${sml_parser}
)

