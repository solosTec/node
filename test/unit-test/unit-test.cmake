# 
#	reset 
#
set (unit_test)

set (unit_test_cpp
	test/unit-test/src/main.cpp
	test/unit-test/src/test-ipt-001.cpp
	test/unit-test/src/test-ipt-002.cpp
	test/unit-test/src/test-ipt-003.cpp
	test/unit-test/src/test-ipt-004.cpp
	test/unit-test/src/test-ipt-005.cpp
	test/unit-test/src/test-sml-001.cpp
	test/unit-test/src/test-sml-002.cpp
	test/unit-test/src/test-sml-003.cpp
	test/unit-test/src/test-sml-004.cpp
#	test/unit-test/src/test-sml-005.cpp
	test/unit-test/src/test-mbus-001.cpp
	test/unit-test/src/test-mbus-002.cpp
	test/unit-test/src/test-mbus-003.cpp
)
    
set (unit_test_h
	test/unit-test/src/test-ipt-001.h
	test/unit-test/src/test-ipt-002.h
	test/unit-test/src/test-ipt-003.h
	test/unit-test/src/test-ipt-004.h
	test/unit-test/src/test-ipt-005.h
	test/unit-test/src/test-sml-001.h
	test/unit-test/src/test-sml-002.h
	test/unit-test/src/test-sml-003.h
	test/unit-test/src/test-sml-004.h
#	test/unit-test/src/test-sml-005.h
	test/unit-test/src/test-mbus-001.h
	test/unit-test/src/test-mbus-002.h
	test/unit-test/src/test-mbus-003.h
)

set (sml_exporter

	src/main/include/smf/sml/exporter/xml_sml_exporter.h
	lib/sml/exporter/src/xml_sml_exporter.cpp
	src/main/include/smf/sml/exporter/db_sml_exporter.h
	lib/sml/exporter/src/db_sml_exporter.cpp
)

set (unit_test_samples
	test/unit-test/src/samples/mbus-003.bin
)

source_group("samples" FILES ${unit_test_samples})

# define the unit test
set (unit_test
  ${unit_test_cpp}
  ${unit_test_h}
  ${sml_exporter}
  ${unit_test_samples}
)

if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	set (unit_test_xml
		${PUGIXML_INCLUDE_DIR}/pugixml.hpp
		${PUGIXML_INCLUDE_DIR}/pugixml.cpp
	)
	list(APPEND unit_test ${unit_test_xml})
	source_group("XML" FILES ${unit_test_xml})

endif()
