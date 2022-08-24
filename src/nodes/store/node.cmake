# 
#	reset 
#
set (store_node)

set (store_cpp
    src/main.cpp
    src/controller.cpp
    src/influxdb.cpp
    src/tasks/network.cpp
)
    
set (store_h
    include/controller.h
    include/influxdb.h
    include/tasks/network.h
)

if(WIN32)
    set(store_assets
		templates/create_service.cmd.in
		templates/delete_service.cmd.in
		templates/restart_service.cmd.in
		templates/windows.cgf.in
    )
else()
    set(store_assets
		templates/unit.in
		templates/linux.cgf.in
    )
endif()

set (store_tasks_sml
    include/tasks/sml_target.h
    include/tasks/sml_db_writer.h
    include/tasks/sml_xml_writer.h
    include/tasks/sml_json_writer.h
    include/tasks/sml_abl_writer.h
    include/tasks/sml_log_writer.h
    include/tasks/sml_csv_writer.h
    include/tasks/sml_influx_writer.h
    src/tasks/sml_target.cpp
    src/tasks/sml_db_writer.cpp
    src/tasks/sml_xml_writer.cpp
    src/tasks/sml_json_writer.cpp
    src/tasks/sml_abl_writer.cpp
    src/tasks/sml_log_writer.cpp
    src/tasks/sml_csv_writer.cpp
    src/tasks/sml_influx_writer.cpp
)

set (store_tasks_iec
    include/tasks/iec_target.h
    include/tasks/iec_db_writer.h
    include/tasks/iec_log_writer.h
    include/tasks/iec_influx_writer.h
    include/tasks/iec_csv_writer.h
    include/tasks/iec_json_writer.h
    src/tasks/iec_target.cpp
    src/tasks/iec_db_writer.cpp
    src/tasks/iec_log_writer.cpp
    src/tasks/iec_influx_writer.cpp
    src/tasks/iec_csv_writer.cpp
    src/tasks/iec_json_writer.cpp
)

set (store_tasks_dlms
    include/tasks/dlms_target.h
    include/tasks/dlms_influx_writer.h
    src/tasks/dlms_target.cpp
    src/tasks/dlms_influx_writer.cpp
)

set(store_tasks_report
    include/tasks/csv_report.h
    include/tasks/lpex_report.h
    src/tasks/csv_report.cpp
    src/tasks/lpex_report.cpp
)

source_group("store-assets" FILES ${store_assets})
source_group("tasks-sml" FILES ${store_tasks_sml})
source_group("tasks-iec" FILES ${store_tasks_iec})
source_group("tasks-dlsm" FILES ${store_tasks_dlms})
source_group("tasks-report" FILES ${store_tasks_report})


set (store_node
  ${store_cpp}
  ${store_h}
  ${store_assets}
  ${store_tasks_sml}
  ${store_tasks_iec}
  ${store_tasks_dlms}
  ${store_tasks_report}
)

