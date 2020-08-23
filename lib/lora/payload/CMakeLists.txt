#
#	LoRa payload library
#
include (lib.cmake)
add_library(smf_lora ${GLOBAL_LIBRARY_TYPE} ${lora_payload_lib})
target_include_directories(smf_lora
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)
