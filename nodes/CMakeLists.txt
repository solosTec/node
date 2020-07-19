#
#	applications
#

#
# master node
#
add_subdirectory(master)

#
# setup node
#
add_subdirectory(setup)

#
# modem node
#
add_subdirectory(modem)

#
# IP-T applications
#
add_subdirectory(ipt)

#
# LoRa
#
if(${PROJECT_NAME}_SSL_SUPPORT)
	add_subdirectory(lora)
endif()

#
# dashboard (HTTP)
#
if(${PROJECT_NAME}_SSL_SUPPORT)
    add_subdirectory(admin)
endif()

if(${PROJECT_NAME}_BUILD_TEST AND ${PROJECT_NAME}_SSL_SUPPORT)
	add_subdirectory(www)
endif()

#
# e350
#
add_subdirectory(e350)



#
# mqtt
# requires MSVC 19 of GCC > 7.x
#
if(MSVC)
	add_subdirectory(mqtt)
endif(MSVC)

#
#	broker
#
add_subdirectory(broker)

#
# tasks
#
add_subdirectory(tasks)

#
# test programs
#
if(${PROJECT_NAME}_BUILD_TEST)
	add_subdirectory(as)
endif()

#
# This program is only for testing purposes
# https://raw.githubusercontent.com/boostorg/beast/develop/example/advanced/server/advanced_server.cpp
#
if(${PROJECT_NAME}_BUILD_TEST AND ${PROJECT_NAME}_SSL_SUPPORT)
#
# This program is only for testing purposes
	add_subdirectory(assl)
endif()


