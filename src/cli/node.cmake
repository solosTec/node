# 
#	reset 
#
set (smf_cli)

set (smf_cli_cpp
    src/main.cpp
    src/analyze_log_v7.cpp
)
    
set (smf_cli_h
    include/analyze_log_v7.h
)

set (smf_cli
  ${smf_cli_cpp}
  ${smf_cli_h}
)

