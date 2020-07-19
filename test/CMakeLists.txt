include (unit-test/unit-test.cmake)
add_executable(unit_test ${unit_test})
target_include_directories(unit_test
	PRIVATE
		"$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/main/include;${CMAKE_BINARY_DIR}>"
		${CYNG_INCLUDE_DIRS}
		${BOOST_INCLUDE_DIR}
		${OPENSSL_INCLUDE_DIR}
		${CRYPT_INCLUDE_DIR}
		${PUGIXML_INCLUDE_DIR}
)
		
# 	set_property(
# 		TARGET unit_test
# 		PROPERTY COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK BOOST_ASIO_HAS_MOVE)

# BOOST_TEST_DYN_LINK is required to build a main() function
target_compile_definitions(unit_test
	PRIVATE
		BOOST_TEST_DYN_LINK BOOST_ASIO_HAS_MOVE)

target_link_libraries(unit_test 
	smf_protocol_ipt 
	smf_bus_ipt 
	smf_protocol_sml 
	smf_protocol_mbus 
	smf_bus_serial
	${CYNG_LIBRARIES}
	${OPENSSL_LIBRARIES}
	"$<$<PLATFORM_ID:Linux>:${SQLite3_LIBRARY};${Boost_UNIT_TEST_FRAMEWORK_LIBRARY};${Boost_LIBRARIES};pthread;${CMAKE_DL_LIBS}>"
	"$<$<PLATFORM_ID:Windows>:odbc32.lib>"
    $<$<AND:$<CXX_COMPILER_VERSION:8.2.1>,$<CXX_COMPILER_ID:GNU>>:stdc++fs>
)
