# top level files
set (lora_payload_lib)

set (lora_payload_cpp

	src/parser.cpp
)

set (lora_payload_h
	${CMAKE_SOURCE_DIR}/src/main/include/smf/lora/payload/parser.h
)

set (lora_data
	src/data/uplinkMsg.xml
	src/data/localisationMsg.xml
)

source_group("data" FILES ${lora_data})


# define the main program
set (lora_payload_lib
  ${lora_payload_cpp}
  ${lora_payload_h}
  ${lora_data}
)

