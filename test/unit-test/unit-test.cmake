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
	test/unit-test/src/test-sml-004.cpp
	test/unit-test/src/test-sml-005.cpp
	test/unit-test/src/test-mbus-001.cpp
	test/unit-test/src/test-mbus-002.cpp
)
    
set (unit_test_h
	test/unit-test/src/test-ipt-001.h
	test/unit-test/src/test-ipt-002.h
	test/unit-test/src/test-ipt-003.h
	test/unit-test/src/test-sml-001.h
	test/unit-test/src/test-sml-002.h
	test/unit-test/src/test-sml-003.h
	test/unit-test/src/test-sml-004.h
	test/unit-test/src/test-sml-005.h
	test/unit-test/src/test-mbus-001.h
	test/unit-test/src/test-mbus-002.h
)

set (sml_exporter

	src/main/include/smf/sml/exporter/xml_sml_exporter.h
	lib/sml/exporter/src/xml_sml_exporter.cpp
	src/main/include/smf/sml/exporter/db_sml_exporter.h
	lib/sml/exporter/src/db_sml_exporter.cpp
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

if (${PROJECT_NAME}_PUGIXML_INSTALLED)
	set (unit_test_xml
		${PUGIXML_INCLUDE_DIR}/pugixml.hpp
		${PUGIXML_INCLUDE_DIR}/pugixml.cpp
	)
	list(APPEND unit_test ${unit_test_xml})
	source_group("XML" FILES ${unit_test_xml})

endif()
