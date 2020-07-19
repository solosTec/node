# top level files
set (ipt_protocol_lib)

set (ipt_protocol_cpp

	src/codes.cpp
	src/header.cpp
	src/response.cpp
	src/scramble_key.cpp
	src/scramble_key_format.cpp
	src/parser.cpp
	src/serializer.cpp
)

set (ipt_protocol_h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/defs.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/codes.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/header.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/response.hpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/scramble_key.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/scramble_key_format.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/scramble_key_io.hpp
	${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/parser.h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/ipt/serializer.h
)


#source_group("shared" FILES ${ipt_shared})

# define the main program
set (ipt_protocol_lib
  ${ipt_protocol_cpp}
  ${ipt_protocol_h}
#  ${ipt_shared}
)

