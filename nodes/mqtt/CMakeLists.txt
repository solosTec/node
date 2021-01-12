
include (prg.cmake)
add_executable(mqtt ${node_mqtt})

target_compile_definitions(mqtt 
	PRIVATE 
		"MQTT_NO_TLS")

target_include_directories(mqtt 
	PRIVATE 
		"${PROJECT_SOURCE_DIR}/3party"
		${CYNG_INCLUDE_DIRS}
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
)
# libraries to link
target_link_libraries(mqtt 
	smf_info 
	smf_cluster 
	${CYNG_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${Boost_THREAD_LIBRARY};${Boost_SYSTEM_LIBRARY};${Boost_FILESYSTEM_LIBRARY};${Boost_PROGRAM_OPTIONS_LIBRARY};${Boost_RANDOM_LIBRARY};${CMAKE_DL_LIBS};${CMAKE_DL_LIBS};pthread>"
)

