# 
#	reset 
#
set (report_lib)

set (report_cpp
    src/lib/report/src/csv.cpp
    src/lib/report/src/lpex.cpp
    src/lib/report/src/gap.cpp
    src/lib/report/src/feed.cpp
    src/lib/report/src/sml_data.cpp
    src/lib/report/src/utility.cpp
)
    
set (report_h
    ${CMAKE_BINARY_DIR}/include/smf.h
    include/smf/report/csv.h   
    include/smf/report/lpex.h   
    include/smf/report/gap.h   
    include/smf/report/feed.h   
    include/smf/report/sml_data.h   
    include/smf/report/utility.h   
    include/smf/report/config/cfg_cleanup.h
)

set(store_config
    include/smf/report/config/cfg_cleanup.h
    include/smf/report/config/cfg_gap.h
    include/smf/report/config/cfg_report.h
    include/smf/report/config/cfg_feed_report.h
    include/smf/report/config/cfg_lpex_report.h
    include/smf/report/config/cfg_csv_report.h
    src/lib/report/src/config/cfg_cleanup.cpp
    src/lib/report/src/config/cfg_gap.cpp
    src/lib/report/src/config/cfg_report.cpp
    src/lib/report/src/config/cfg_feed_report.cpp
    src/lib/report/src/config/cfg_lpex_report.cpp
    src/lib/report/src/config/cfg_csv_report.cpp
)

source_group("config" FILES ${store_config})

# define the report lib
set (report_lib
  ${report_cpp}
  ${report_h}
  ${store_config}
)

