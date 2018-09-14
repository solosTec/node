# top level files
set (mbus_protocol_lib)

set (mbus_protocol_cpp

	lib/mbus/protocol/src/parser.cpp
	lib/mbus/protocol/src/variable_data_block.cpp
	lib/mbus/protocol/src/units.cpp
)

set (mbus_protocol_h
	src/main/include/smf/mbus/defs.h
	src/main/include/smf/mbus/parser.h
	src/main/include/smf/mbus/variable_data_block.h
	src/main/include/smf/mbus/units.h
)

# define the main program
set (mbus_protocol_lib
  ${mbus_protocol_cpp}
  ${mbus_protocol_h}
)

