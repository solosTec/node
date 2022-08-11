# 
#	reset 
#
set (report_lib)

set (report_cpp
    src/lib/report/src/csv.cpp
    src/lib/report/src/lpex.cpp
    src/lib/report/src/sml_data.cpp
    src/lib/report/src/utility.cpp
)
    
set (report_h
    ${CMAKE_BINARY_DIR}/include/smf.h
    include/smf/report/csv.h   
    include/smf/report/lpex.h   
    include/smf/report/sml_data.h   
    include/smf/report/utility.h   
)

# define the report lib
set (report_lib
  ${report_cpp}
  ${report_h}
)

