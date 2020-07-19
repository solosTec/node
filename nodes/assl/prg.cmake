# top level files
set (node_assl)

set (node_assl_cpp

	src/main.cpp	
)

set (node_assl_h

	src/server_certificate.hpp
	src/detect_ssl.hpp
)

# define the main program
set (node_assl
  ${node_assl_cpp}
  ${node_assl_h}
)

