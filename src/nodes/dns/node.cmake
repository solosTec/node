# 
#	reset 
#
set (dns_node)

set (dns_cpp
    src/main.cpp
    src/controller.cpp
    src/server.cpp
)
    
set (dns_h
    include/controller.h
    include/server.h
)

set (dns_tasks
    include/tasks/cluster.h
    src/tasks/cluster.cpp
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
source_group("tasks" FILES ${dns_tasks})


set (dns_node
  ${dns_cpp}
  ${dns_h}
  ${dns_assets}
  ${dns_tasks}
)

