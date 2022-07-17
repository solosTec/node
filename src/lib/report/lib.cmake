# 
#	reset 
#
set (report_lib)

set (report_cpp
    src/lib/report/src/report.cpp
    src/lib/report/src/sml_data.cpp
)
    
set (report_h
    ${CMAKE_BINARY_DIR}/include/smf.h
    include/smf/report/report.h   
    include/smf/report/sml_data.h   
)

# define the report lib
set (report_lib
  ${report_cpp}
  ${report_h}
)

