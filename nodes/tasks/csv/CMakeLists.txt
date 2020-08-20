#
# task: csv
#
include (prg.cmake)
add_executable(csv ${task_csv})
target_include_directories(csv
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(csv 
	smf_info 
	smf_protocol_sml
	smf_cluster 
	${CYNG_LIBRARIES}
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};${SQLite3_LIBRARY};pthread>"
	$<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	$<$<AND:$<VERSION_LESS:${CMAKE_CXX_COMPILER_VERSION},9.0>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>   
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)
