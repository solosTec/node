# top level files
set (sml_protocol_lib)

set (sml_protocol_cpp

	src/crc16.cpp
#	src/units.cpp
	src/scaler.cpp
	src/writer.hpp
	src/writer.cpp
	src/mbus_defs.cpp
	src/obis_db.cpp
	src/obis_io.cpp
	src/srv_id_io.cpp
	src/ip_io.cpp
	# moved from bus
	src/status.cpp
	src/event.cpp
)

set (sml_protocol_h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/defs.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/crc16.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/scaler.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/mbus/defs.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/obis_db.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/obis_io.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/srv_id_io.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/ip_io.h
	# moved from bus
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/status.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/event.h
)

set (sml_generator
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/protocol/parser.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/protocol/readout.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/protocol/serializer.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/protocol/message.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/protocol/generator.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/protocol/value.hpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/protocol/reader.h

	src/parser.cpp
	src/readout.cpp
	src/serializer.cpp
	src/message.cpp
	src/generator.cpp
	src/value.cpp
	src/reader.cpp
)

set (sml_parser
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/parser/obis_parser.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/parser/srv_id_parser.h

	src/parser/obis_parser.cpp
	src/parser/obis_parser.hpp
	src/parser/srv_id_parser.cpp
	src/parser/srv_id_parser.hpp
)

set (sml_intrinsics
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/intrinsics/obis.h
	src/intrinsics/obis.cpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/sml/intrinsics/obis_factory.hpp
	src/intrinsics/obis_factory.cpp
)

source_group("generator" FILES ${sml_generator})
source_group("parser" FILES ${sml_parser})
source_group("intrinsics" FILES ${sml_intrinsics})


# define the main program
set (sml_protocol_lib
  ${sml_protocol_cpp}
  ${sml_protocol_h}
  ${sml_generator}
  ${sml_parser}
  ${sml_intrinsics}
)

