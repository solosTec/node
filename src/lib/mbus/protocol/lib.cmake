# 
#	reset 
#
set (mbus_lib)

set (mbus_cpp
    src/lib/mbus/protocol/src/units.cpp
    src/lib/mbus/protocol/src/medium.cpp
    src/lib/mbus/protocol/src/aes.cpp
    src/lib/mbus/protocol/src/flag_id.cpp
    src/lib/mbus/protocol/src/dif.cpp
    src/lib/mbus/protocol/src/vif.cpp
    src/lib/mbus/protocol/src/server_id.cpp
    src/lib/mbus/protocol/src/reader.cpp
    src/lib/mbus/protocol/src/field_definitions.cpp
)
    
set (mbus_h
    include/smf/mbus.h    
    include/smf/mbus/field_definitions.h
    include/smf/mbus/units.h
    include/smf/mbus/medium.h
    include/smf/mbus/aes.h
    include/smf/mbus/bcd.hpp
    include/smf/mbus/flag_id.h
    include/smf/mbus/dif.h
    include/smf/mbus/vif.h
    include/smf/mbus/server_id.h
    include/smf/mbus/reader.h
)

set (mbus_radio
    include/smf/mbus/radio/header.h
    include/smf/mbus/radio/parser.h
    include/smf/mbus/radio/decode.h
    src/lib/mbus/protocol/src/radio/header.cpp
    src/lib/mbus/protocol/src/radio/parser.cpp
    src/lib/mbus/protocol/src/radio/decode.cpp
)
set (mbus_wired
)

source_group("wireless" FILES ${mbus_radio})
source_group("wired" FILES ${mbus_wired})

# define the mbus lib
set (mbus_lib
  ${mbus_cpp}
  ${mbus_h}
  ${mbus_radio}
  ${mbus_wired}
)

