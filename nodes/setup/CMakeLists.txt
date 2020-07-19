#
# setup
#
include (prg.cmake)
add_executable(setup ${node_setup})
target_include_directories(setup
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${CRYPT_INCLUDE_DIR}
		${OPENSSL_INCLUDE_DIR}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(setup 
	smf_info 
	smf_cluster 
	${SMF_CRYPTO_LIBRARY}
	${CYNG_LIBRARIES}
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};${SQLite3_LIBRARY};pthread>"
	$<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
)
