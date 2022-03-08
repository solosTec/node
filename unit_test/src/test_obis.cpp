#ifdef STAND_ALONE
#define BOOST_TEST_MODULE unit_test
#endif

#include <boost/test/unit_test.hpp>
#include <smf/obis/defs.h>
#include <smf/obis/list.h>
#include <smf/obis/tree.hpp>
#include <smf/sml/value.hpp>

#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/obj/factory.hpp>
#include <cyng/obj/value_cast.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(obis_suite)

BOOST_AUTO_TEST_CASE(tree) {
    smf::sml::tree t;
    cyng::obis_path_t const p1{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME, smf::OBIS_NMS_PORT};
    t.add(p1, smf::sml::make_attribute("METER_ADDRESS/ACCESS_USER_NAME/NMS_PORT"));

    cyng::obis_path_t const p2{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME, smf::OBIS_NMS_ADDRESS};
    t.add(p2, smf::sml::make_attribute("METER_ADDRESS/ACCESS_USER_NAME/NMS_ADDRESS"));

    cyng::obis_path_t const p3{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME, smf::OBIS_COMPUTER_NAME};
    t.add(p3, smf::sml::make_attribute("METER_ADDRESS/ACCESS_USER_NAME/COMPUTER_NAME"));

    cyng::obis_path_t const p4{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME};
    t.add(p4, smf::sml::make_attribute("METER_ADDRESS/ACCESS_USER_NAME"));

    cyng::obis_path_t const p5{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME, smf::OBIS_CURRENT_UTC};
    t.add(
        p5,
        smf::sml::make_timestamp_attr(std::chrono::system_clock::from_time_t(1643798358))); //  Wed Feb 02 2022 10:39:18 GMT+0000

    auto const attr1 = t.find(p1);
    // std::cout << obj1 << std::endl;
    cyng::buffer_t tmp;
    tmp = cyng::value_cast(attr1.second, tmp);
    // std::cout << cyng::value_cast<std::string>(obj1, "") << std::endl;
    BOOST_REQUIRE_EQUAL(std::string(tmp.begin(), tmp.end()), "METER_ADDRESS/ACCESS_USER_NAME/NMS_PORT");

    auto const attr2 = t.find(p2);
    // std::cout << obj2 << std::endl;
    tmp = cyng::value_cast(attr2.second, tmp);
    BOOST_REQUIRE_EQUAL(std::string(tmp.begin(), tmp.end()), "METER_ADDRESS/ACCESS_USER_NAME/NMS_ADDRESS");

    auto const obj3 = t.find(p3);
    // std::cout << obj3 << std::endl;
    tmp = cyng::value_cast(obj3.second, tmp);
    BOOST_REQUIRE_EQUAL(std::string(tmp.begin(), tmp.end()), "METER_ADDRESS/ACCESS_USER_NAME/COMPUTER_NAME");

    auto const attr4 = t.find(p4);
    // std::cout << obj4 << std::endl;
    tmp = cyng::value_cast(attr4.second, tmp);
    BOOST_REQUIRE_EQUAL(std::string(tmp.begin(), tmp.end()), "METER_ADDRESS/ACCESS_USER_NAME");

    auto const attr5 = t.find(p5);
    std::cout << attr5 << std::endl;
    auto tp = cyng::value_cast<std::uint32_t>(attr5.second, 0);
    BOOST_CHECK_EQUAL(tp, 1643798358);

    auto const cl1 = t.to_child_list();
    // std::cout << cyng::io::to_typed(cl1) << std::endl;
    std::cout << cyng::io::to_pretty(cl1) << std::endl;
    std::cout << smf::sml::dump_child_list(cl1, false);

    cyng::obis_path_t psub1{smf::OBIS_METER_ADDRESS};
    auto const cl2 = t.get_subtree(psub1).to_child_list();
    std::cout << cyng::io::to_pretty(cl2) << std::endl;

    cyng::obis_path_t psub2{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME};
    auto const s1 = t.size(psub2);
    // std::cout << "size: " << s1 << std::endl;
    BOOST_REQUIRE_EQUAL(s1, 4);
}

BOOST_AUTO_TEST_CASE(otree) {
    smf::sml::obis_tree<std::string> ot{};
    cyng::obis_path_t const p1{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME, smf::OBIS_NMS_PORT};
    ot.add(p1, "METER_ADDRESS / ACCESS_USER_NAME / NMS_PORT");

    cyng::obis_path_t const p2{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME, smf::OBIS_NMS_ADDRESS};
    ot.add(p2, "METER_ADDRESS / ACCESS_USER_NAME / NMS_ADDRESS");

    cyng::obis_path_t const p3{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME, smf::OBIS_COMPUTER_NAME};
    ot.add(p3, "METER_ADDRESS / ACCESS_USER_NAME / COMPUTER_NAME");

    cyng::obis_path_t const p4{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME};
    ot.add(p4, "METER_ADDRESS / ACCESS_USER_NAME");

    auto const v1 = ot.find(p1);
    std::cout << v1 << std::endl;
    BOOST_CHECK_EQUAL(v1, "METER_ADDRESS / ACCESS_USER_NAME / NMS_PORT");

    cyng::obis_path_t psub2{smf::OBIS_METER_ADDRESS, smf::OBIS_ACCESS_USER_NAME};
    auto const s1 = ot.size(psub2);
    std::cout << "size: " << s1 << std::endl;
    BOOST_REQUIRE_EQUAL(s1, 3);

    auto const tpl1 = ot.to_tuple();
    std::cout << cyng::io::to_pretty(tpl1) << std::endl;

    auto const tpl2 = ot.to_tuple<cyng::tuple_t>([](std::string s) -> cyng::tuple_t { return cyng::make_tuple(s + " <==> " + s); });
    std::cout << cyng::io::to_pretty(tpl2) << std::endl;

    //  convert to cyng::prop_map_t
    auto const prop1 = ot.to_prop_map();
    std::cout << prop1 << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
