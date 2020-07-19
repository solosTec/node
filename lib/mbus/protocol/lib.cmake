# top level files
set (mbus_protocol_lib)

set (mbus_protocol_cpp

	src/parser.cpp
	src/variable_data_block.cpp
	src/header.cpp
	src/units.cpp
	src/aes.cpp
	src/dif.cpp
	src/vif.cpp
	src/bcd.cpp
)

set (mbus_protocol_h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/mbus/defs.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/mbus/parser.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/mbus/variable_data_block.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/mbus/header.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/mbus/units.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/mbus/aes.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/mbus/dif.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/mbus/vif.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/mbus/bcd.h
)

# define the main program
set (mbus_protocol_lib
  ${mbus_protocol_cpp}
  ${mbus_protocol_h}
)

