include (prg.cmake)
add_executable(assl ${node_assl})
target_include_directories(assl
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${OPENSSL_INCLUDE_DIR}
#			${BOOST_INCLUDE_DIR}
)
target_link_libraries(assl 
	${OPENSSL_LIBRARIES}
#		${SMF_CRYPTO_LIBRARY}
	"$<$<PLATFORM_ID:Linux>:${Boost_LIBRARIES};pthread>"
)	
