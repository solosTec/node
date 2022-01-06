# 
#	reset 
#
set (dns_node)

set (dns_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (dns_h
    include/controller.h
#    include/dns/header.h
)

if(WIN32)
    set(dns_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(dns_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()


source_group("dns-assets" FILES ${dns_assets})


set (dns_node
  ${dns_cpp}
  ${dns_h}
  ${dns_assets}
)

