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

cmake_host_system_information(RESULT _HOST_NUMBER_OF_PHYSICAL_CORES QUERY NUMBER_OF_PHYSICAL_CORES)

if (${_HOST_NUMBER_OF_PHYSICAL_CORES} LESS 4) 
	set(${PROJECT_NAME}_POOL_SIZE 4)
else()
	set(${PROJECT_NAME}_POOL_SIZE 4)
endif()
