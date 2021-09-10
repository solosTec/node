#ifdef STAND_ALONE
#define BOOST_TEST_MODULE unit_test
#endif

#include <boost/test/unit_test.hpp>
#include <smf/obis/defs.h>
#include <smf/obis/tree.h>

#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/obj/factory.hpp>
#include <cyng/obj/value_cast.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(obis_suite)

BOOST_AUTO_TEST_CASE(tree) {
    smf::sml::tree t;
    cyng::obis_path_t p1{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME, smf::OBIS_NMS_PORT};
    t.add(p1, cyng::make_object("METER_ADDRESS/ACCESS_USER_NAME/NMS_PORT"));

    cyng::obis_path_t p2{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME, smf::OBIS_NMS_ADDRESS};
    t.add(p2, cyng::make_object("METER_ADDRESS/ACCESS_USER_NAME/NMS_ADDRESS"));

    cyng::obis_path_t p3{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME, smf::OBIS_COMPUTER_NAME};
    t.add(p3, cyng::make_object("METER_ADDRESS/ACCESS_USER_NAME/COMPUTER_NAME"));

    cyng::obis_path_t p4{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME};
    t.add(p4, cyng::make_object("METER_ADDRESS/ACCESS_USER_NAME"));

    auto const obj1 = t.find(p1);
    // std::cout << obj1 << std::endl;
    auto const str1 = cyng::value_cast<std::string>(obj1, "");
    // std::cout << cyng::value_cast<std::string>(obj1, "") << std::endl;
    BOOST_REQUIRE_EQUAL(str1, "METER_ADDRESS/ACCESS_USER_NAME/NMS_PORT");

    auto const obj2 = t.find(p2);
    // std::cout << obj2 << std::endl;
    auto const str2 = cyng::value_cast<std::string>(obj2, "");
    BOOST_REQUIRE_EQUAL(str2, "METER_ADDRESS/ACCESS_USER_NAME/NMS_ADDRESS");

    auto const obj3 = t.find(p3);
    // std::cout << obj3 << std::endl;
    auto const str3 = cyng::value_cast<std::string>(obj3, "");
    BOOST_REQUIRE_EQUAL(str3, "METER_ADDRESS/ACCESS_USER_NAME/COMPUTER_NAME");

    auto const obj4 = t.find(p4);
    // std::cout << obj4 << std::endl;
    auto const str4 = cyng::value_cast<std::string>(obj4, "");
    BOOST_REQUIRE_EQUAL(str4, "METER_ADDRESS/ACCESS_USER_NAME");

    auto const cl1 = t.to_child_list();
    std::cout << cyng::io::to_typed(cl1) << std::endl;

    cyng::obis_path_t psub1{smf::OBIS_METER_ADDRESS};
    auto const cl2 = t.get_subtree(psub1).to_child_list();
    std::cout << cl2 << std::endl;

    cyng::obis_path_t psub2{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME};
    auto const s1 = t.size(psub2);
    // std::cout << "size: " << s1 << std::endl;
    BOOST_REQUIRE_EQUAL(s1, 3);
}

BOOST_AUTO_TEST_SUITE_END()
