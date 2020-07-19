# top level files
set (modem_protocol_lib)

set (modem_protocol_cpp

	src/parser.cpp
	src/serializer.cpp
)

set (modem_protocol_h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/modem/parser.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/modem/serializer.h
)


#source_group("shared" FILES ${modem_shared})

# define the main program
set (modem_protocol_lib
  ${modem_protocol_cpp}
  ${modem_protocol_h}
)

