#
#	detect address model
#
if(CMAKE_SIZEOF_VOID_P EQUAL 8)

	message(STATUS "** Address Model      : 64 bit")
	set(${PROJECT_NAME}_ADDRESS_MODEL 64)

elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)

	message(STATUS "** Address Model      : 32 bit")
	set(${PROJECT_NAME}_ADDRESS_MODEL 32)

else()

	message(WARNING "** Address Model      : not supported")
	set(${PROJECT_NAME}_ADDRESS_MODEL 16)

endif()

