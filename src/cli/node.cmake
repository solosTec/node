# 
#	reset 
#
set (smf_cli)

set (smf_cli_cpp
    src/main.cpp
    src/analyze_log_v7.cpp
    src/analyze_log_v8.cpp
    src/send_tcp_ip.cpp
    src/transfer_to_db.cpp
)
    
set (smf_cli_h
    include/analyze_log_v7.h
    include/analyze_log_v8.h
    include/send_tcp_ip.h
    include/transfer_to_db.h
)

set (smf_cli
  ${smf_cli_cpp}
  ${smf_cli_h}
)

