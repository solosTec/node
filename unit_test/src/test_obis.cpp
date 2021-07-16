#ifdef STAND_ALONE
#define BOOST_TEST_MODULE unit_test
#endif

#include <boost/test/unit_test.hpp>
#include <smf/obis/defs.h>
#include <smf/obis/tree.h>

#include <cyng/io/ostream.h>
#include <cyng/obj/factory.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(obis_suite)

BOOST_AUTO_TEST_CASE(tree) {
    smf::obis::tree t;
    cyng::obis_path_t p1{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME, smf::OBIS_NMS_PORT};
    t.add(p1, cyng::make_object("METER_ADDRESS/ACCESS_USER_NAME/NMS_PORT"));

    cyng::obis_path_t p2{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME, smf::OBIS_NMS_ADDRESS};
    t.add(p2, cyng::make_object("METER_ADDRESS/ACCESS_USER_NAME/NMS_ADDRESS"));

    cyng::obis_path_t p3{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME, smf::OBIS_COMPUTER_NAME};
    t.add(p3, cyng::make_object("METER_ADDRESS/ACCESS_USER_NAME/COMPUTER_NAME"));

    cyng::obis_path_t p4{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME};
    t.add(p4, cyng::make_object("METER_ADDRESS/ACCESS_USER_NAME"));

    auto const obj1 = t.find(p1);
    std::cout << obj1 << std::endl;

    auto const obj2 = t.find(p2);
    std::cout << obj2 << std::endl;

    auto const obj3 = t.find(p3);
    std::cout << obj3 << std::endl;

    auto const obj4 = t.find(p4);
    std::cout << obj4 << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
