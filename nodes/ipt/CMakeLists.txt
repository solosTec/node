add_subdirectory(collector)
add_subdirectory(emitter)
add_subdirectory(master)
add_subdirectory(segw)
add_subdirectory(store)
if(${PROJECT_NAME}_BUILD_TEST)
	#
	# This program is only for testing purposes
	# IP-T stress
	#
	add_subdirectory(stress)
endif()
