# top level files
set (imega_protocol_lib)

set (imega_protocol_cpp

	src/parser.cpp
	src/serializer.cpp
)

set (imega_protocol_h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/imega/parser.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/imega/serializer.h
)


#source_group("shared" FILES ${imega_shared})

# define the main program
set (imega_protocol_lib
  ${imega_protocol_cpp}
  ${imega_protocol_h}
)

