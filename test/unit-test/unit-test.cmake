# 
#	reset 
#
set (unit_test)

set (unit_test_cpp
	test/unit-test/src/main.cpp
	test/unit-test/src/test-ipt-001.cpp
	test/unit-test/src/test-ipt-002.cpp
	test/unit-test/src/test-ipt-003.cpp
	test/unit-test/src/test-sml-001.cpp
	test/unit-test/src/test-sml-002.cpp
	test/unit-test/src/test-sml-003.cpp
)
    
set (unit_test_h
	test/unit-test/src/test-ipt-001.h
	test/unit-test/src/test-ipt-002.h
	test/unit-test/src/test-ipt-003.h
	test/unit-test/src/test-sml-001.h
	test/unit-test/src/test-sml-002.h
	test/unit-test/src/test-sml-003.h
)

set (sml_exporter

	src/main/include/smf/sml/exporter/xml_exporter.h
	lib/sml/exporter/src/xml_exporter.cpp
	src/main/include/smf/sml/exporter/db_exporter.h
	lib/sml/exporter/src/db_exporter.cpp
#	lib/sml/exporter/src/abl.cpp
#	lib/sml/exporter/src/json.cpp
#	lib/sml/exporter/src/db.cpp

)


# define the unit test
set (unit_test
  ${unit_test_cpp}
  ${unit_test_h}
  ${sml_exporter}
)

