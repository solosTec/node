#
#	HCI/CP210x protocol library
#
include (lib.cmake)
add_library(smf_protocol_hci ${GLOBAL_LIBRARY_TYPE} ${hci_protocol_lib})
target_include_directories(smf_protocol_hci
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
)
