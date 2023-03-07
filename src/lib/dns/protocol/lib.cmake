# 
#	reset 
#
set (dns_lib)

set (dns_cpp
    src/lib/dns/protocol/src/parser.cpp
    src/lib/dns/protocol/src/serializer.cpp
)
    
set (dns_h
#    include/smf/dns.h
    include/smf/dns/header.h
    include/smf/dns/parser.h
    include/smf/dns/serializer.h
)


# define the dns lib
set (dns_lib
  ${dns_cpp}
  ${dns_h}
)

