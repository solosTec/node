# top level files
set (mbus_protocol_lib)

set (mbus_protocol_cpp

	lib/mbus/protocol/src/parser.cpp
	lib/mbus/protocol/src/variable_data_block.cpp
	lib/mbus/protocol/src/header.cpp
	lib/mbus/protocol/src/units.cpp
	lib/mbus/protocol/src/aes.cpp
	lib/mbus/protocol/src/dif.cpp
	lib/mbus/protocol/src/vif.cpp
	lib/mbus/protocol/src/bcd.cpp
)

set (mbus_protocol_h
	src/main/include/smf/mbus/defs.h
	src/main/include/smf/mbus/parser.h
	src/main/include/smf/mbus/variable_data_block.h
	src/main/include/smf/mbus/header.h
	src/main/include/smf/mbus/units.h
	src/main/include/smf/mbus/aes.h
	src/main/include/smf/mbus/dif.h
	src/main/include/smf/mbus/vif.h
	src/main/include/smf/mbus/bcd.h
)

# define the main program
set (mbus_protocol_lib
  ${mbus_protocol_cpp}
  ${mbus_protocol_h}
)

