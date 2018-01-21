# 
#	reset 
#
set (unit_test)

set (unit_test_cpp
	test/unit-test/src/main.cpp
	test/unit-test/src/test-ipt-001.cpp
	test/unit-test/src/test-ipt-002.cpp
	test/unit-test/src/test-ipt-003.cpp
)
    
set (unit_test_h
	test/unit-test/src/test-ipt-001.h
	test/unit-test/src/test-ipt-002.h
	test/unit-test/src/test-ipt-003.h
)

# define the unit test
set (unit_test
  ${unit_test_cpp}
  ${unit_test_h}
)

