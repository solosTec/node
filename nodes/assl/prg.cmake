# top level files
set (node_assl)

set (node_assl_cpp

	nodes/assl/src/main.cpp	
)

set (node_assl_h

	nodes/assl/src/server_certificate.hpp
	nodes/assl/src/detect_ssl.hpp
)

# define the main program
set (node_assl
  ${node_assl_cpp}
  ${node_assl_h}
)

