#
# IP-T emitter
#
include (prg.cmake)
add_executable(emitter ${node_ipt_emitter})
target_include_directories(emitter
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(emitter 
	smf_info 
	smf_protocol_ipt
	smf_bus_ipt
	${CYNG_LIBRARIES} 
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};pthread>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)

