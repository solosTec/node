#
# task: stat (statistics - detecting gaps, etc)
#
include (prg.cmake)
add_executable(stat ${task_stat})
target_include_directories(stat
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)
target_link_libraries(stat 
	smf_info 
	smf_cluster 
	${OPENSSL_LIBRARIES}
	${CYNG_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_LIBRARIES};pthread;${CMAKE_DL_LIBS}>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)
