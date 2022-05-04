# Definition of platform dependend path names
#
# Variables used::
#
#   CLS_PATH_APP
#   CLS_PATH_SCRIPT
#   CLS_PATH_ETC
#   CLS_PATH_CONFIG
#
#   A missing platform definition will silently ignored.

if(NOT PLATFORM)
    message(STATUS "** Platform                 : default (OECP-1)")
else()
    message(STATUS "** Platform                 : ${PLATFORM}")
endif()

if ("${PLATFORM}" STREQUAL "arm-cls") 
    set(CLS_PATH_ROOT "/usr")
    set(CLS_PATH_BIN "/usr/bin")
	set(CLS_PATH_SCRIPT "/usr/sbin")
	set(CLS_PATH_ETC "/etc")
	set(CLS_PATH_CONFIG "/etc/cls")
else()
    set(CLS_PATH_ROOT "/usr/local")
    set(CLS_PATH_BIN "/usr/bin")
	set(CLS_PATH_SCRIPT "/usr/local/sbin")
	set(CLS_PATH_ETC "/usr/local/etc")
	set(CLS_PATH_CONFIG "/usr/local/etc/CLS")
endif()

add_compile_definitions(
	"_CLS_PATH_ROOT=\"${CLS_PATH_ROOT}\""
	"_CLS_PATH_BIN=\"${CLS_PATH_BIN}\""
	"_CLS_PATH_SCRIPT=\"${CLS_PATH_SCRIPT}\""
	"_CLS_PATH_ETC=\"${CLS_PATH_ETC}\""
	"_CLS_PATH_CONFIG=\"${CLS_PATH_CONFIG}\""
)