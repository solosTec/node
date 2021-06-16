# 
#	reset 
#
set (sml_lib)

set (sml_cpp
    src/lib/sml/protocol/src/sml.cpp
    src/lib/sml/protocol/src/tokenizer.cpp
    src/lib/sml/protocol/src/parser.cpp
    src/lib/sml/protocol/src/unpack.cpp
    src/lib/sml/protocol/src/crc16.cpp
    src/lib/sml/protocol/src/status.cpp
    src/lib/sml/protocol/src/event.cpp
)
    
set (sml_h
    include/smf/sml.h
    include/smf/sml/tokenizer.h    
    include/smf/sml/parser.h
    include/smf/sml/unpack.h
    include/smf/sml/crc16.h
    include/smf/sml/status.h
    include/smf/sml/event.h
)

set (sml_producer
    include/smf/sml/msg.h
    include/smf/sml/value.hpp
    include/smf/sml/writer.hpp
    include/smf/sml/generator.h
    src/lib/sml/protocol/src/msg.cpp
    src/lib/sml/protocol/src/value.cpp
    src/lib/sml/protocol/src/writer.cpp
    src/lib/sml/protocol/src/generator.cpp
)

set (sml_consumer
    include/smf/sml/reader.h
    include/smf/sml/readout.h
    src/lib/sml/protocol/src/reader.cpp
    src/lib/sml/protocol/src/readout.cpp
)

source_group("producer" FILES ${sml_producer})
source_group("consumer" FILES ${sml_consumer})


# define the sml lib
set (sml_lib
    ${sml_cpp}
    ${sml_h}
    ${sml_producer}
    ${sml_consumer}
)

