# 
#	reset 
#
set (report_node)

set (report_cpp
    src/main.cpp
    src/controller.cpp
)
    
set (report_h
    include/controller.h
)

if(WIN32)
    set(report_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(report_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()

set (report_tasks
  include/tasks/cluster.h
  include/tasks/report.h
  src/tasks/cluster.cpp
  src/tasks/report.cpp
)

source_group("report-assets" FILES ${report_assets})
source_group("tasks" FILES ${report_tasks})


set (report_node
  ${report_cpp}
  ${report_h}
  ${report_assets}
  ${report_tasks}
)

