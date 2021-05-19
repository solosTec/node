# 
#	reset 
#
set (store_node)

set (store_cpp
    src/main.cpp
    src/controller.cpp
    src/influxdb.cpp
)
    
set (store_h
    include/controller.h
    include/influxdb.h
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

set (store_tasks
  include/tasks/network.h
  include/tasks/sml_target.h
  include/tasks/iec_target.h
  include/tasks/dlms_target.h
  include/tasks/sml_db_writer.h
  include/tasks/sml_xml_writer.h
  include/tasks/sml_json_writer.h
  include/tasks/sml_abl_writer.h
  include/tasks/sml_log_writer.h
  include/tasks/sml_csv_writer.h
  include/tasks/sml_influx_writer.h
  include/tasks/iec_db_writer.h
  include/tasks/iec_log_writer.h
  include/tasks/iec_influx_writer.h
  include/tasks/iec_csv_writer.h
  include/tasks/dlms_influx_writer.h
  src/tasks/network.cpp
  src/tasks/sml_target.cpp
  src/tasks/iec_target.cpp
  src/tasks/dlms_target.cpp
  src/tasks/sml_db_writer.cpp
  src/tasks/sml_xml_writer.cpp
  src/tasks/sml_json_writer.cpp
  src/tasks/sml_abl_writer.cpp
  src/tasks/sml_log_writer.cpp
  src/tasks/sml_csv_writer.cpp
  src/tasks/sml_influx_writer.cpp
  src/tasks/iec_db_writer.cpp
  src/tasks/iec_log_writer.cpp
  src/tasks/iec_influx_writer.cpp
  src/tasks/iec_csv_writer.cpp
  src/tasks/dlms_influx_writer.cpp
)

source_group("store-assets" FILES ${store_assets})
source_group("tasks" FILES ${store_tasks})


set (store_node
  ${store_cpp}
  ${store_h}
  ${store_assets}
  ${store_tasks}
)

