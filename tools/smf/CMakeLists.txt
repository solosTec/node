include (prg.cmake)
add_executable(smf ${tool_smf})
target_include_directories(smf
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
		${CRYPT_INCLUDE_DIR}
		${PUGIXML_INCLUDE_DIR}
)
# libraries to link 
target_link_libraries(smf 
	smf_info 
	smf_protocol_sml 
	smf_protocol_mbus 
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_LIBRARIES};pthread;readline>"
	$<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	$<$<AND:$<VERSION_LESS:${CMAKE_CXX_COMPILER_VERSION},9.0>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>   
)
