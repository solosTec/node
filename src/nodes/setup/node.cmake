# 
#	reset 
#
set (setup_node)

set (setup_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (setup_h
    include/controller.h
)

if(WIN32)
    set(setup_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(setup_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()

set (setup_tasks
  include/tasks/cluster.h
  include/tasks/storage_db.h
  include/tasks/storage_json.h
  include/tasks/storage_xml.h
#  include/tasks/sync.h
  src/tasks/cluster.cpp
  src/tasks/storage_db.cpp
  src/tasks/storage_json.cpp
  src/tasks/storage_xml.cpp
#  src/tasks/sync.cpp
)

source_group("setup-assets" FILES ${setup_assets})
source_group("tasks" FILES ${setup_tasks})


set (setup_node
  ${setup_cpp}
  ${setup_h}
  ${setup_assets}
  ${setup_tasks}
)

