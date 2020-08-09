#
# LoRa
#

include (prg.cmake)
add_executable(lora ${node_lora})
target_include_directories(lora
	PRIVATE
	    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
	    ${CYNG_INCLUDE_DIRS}
		${OPENSSL_INCLUDE_DIR}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(lora 
	smf_info 
	smf_cluster 
	smf_https_srv 
	smf_lora 
	smf_protocol_sml
	${OPENSSL_LIBRARIES}
	${SMF_CRYPTO_LIBRARY}
	${CYNG_LIBRARIES} 
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};pthread>"
	$<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	$<$<AND:$<VERSION_LESS:${CMAKE_CXX_COMPILER_VERSION},9.0>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>   
)


