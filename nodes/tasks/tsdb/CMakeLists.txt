
#
# task: tsdb (time series database)
#
include (prg.cmake)
add_executable(tsdb ${task_tsdb})
target_include_directories(tsdb
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
)

#message(STATUS "** Boost LibDir    : ${Boost_LIBRARY_DIRS}")

target_link_directories(tsdb
    PRIVATE
		${Boost_LIBRARY_DIRS}
)

target_link_libraries(tsdb 
	${Boost_LIBRARIES}
	smf_info 
	smf_cluster 
	${CYNG_LIBRARIES}
	${OPENSSL_LIBRARIES}
#	Error	LNK1104	cannot open file 'libboost_date_time-vc142-mt-gd-x64-1_74.lib'	tsdb	D:\projects\node\build\nodes\tasks\tsdb\LINK	1	
#	"$<$<PLATFORM_ID:Windows>:C:/local/boost_1_75_0/lib64-msvc-14.2/libboost_date_time-vc142-mt-x64-1_75.lib>"
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};pthread>"
	$<$<AND:$<VERSION_LESS_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
	$<$<AND:$<VERSION_LESS:${CMAKE_CXX_COMPILER_VERSION},9.0>,$<CXX_COMPILER_ID:Clang>>:stdc++fs>   
#	Boost::date_time 
#	${Boost_LIBRARIES}
)
