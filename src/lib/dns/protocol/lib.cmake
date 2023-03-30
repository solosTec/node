# 
#	reset 
#
set (dns_lib)

set (dns_cpp
    src/lib/dns/protocol/src/msg.cpp
    src/lib/dns/protocol/src/parser.cpp
    src/lib/dns/protocol/src/serializer.cpp
    src/lib/dns/protocol/src/op_code.cpp
    src/lib/dns/protocol/src/r_code.cpp
)
    
set (dns_h
#    include/smf/dns.h
    include/smf/dns/msg.h
    include/smf/dns/parser.h
    include/smf/dns/serializer.h
    include/smf/dns/op_code.h
    include/smf/dns/r_code.h
)


# define the dns lib
set (dns_lib
  ${dns_cpp}
  ${dns_h}
)

