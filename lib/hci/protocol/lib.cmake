# top level files
set (hci_protocol_lib)

set (hci_protocol_cpp

	src/parser.cpp
#	src/serializer.cpp
)

set (hci_protocol_h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/hci/parser.h
#	src/main/include/smf/hci/serializer.h
)

# define the main program
set (hci_protocol_lib
  ${hci_protocol_cpp}
  ${hci_protocol_h}
)

