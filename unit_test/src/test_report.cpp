#ifdef STAND_ALONE
#define BOOST_TEST_MODULE unit_test
#endif

#include <boost/test/unit_test.hpp>

#include <smf/obis/defs.h>
#include <smf/obis/profile.h>
#include <smf/report/utility.h>

#include <cyng/sys/clock.h>

#ifdef _DEBUG
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>
#endif

BOOST_AUTO_TEST_SUITE(report_suite)

BOOST_AUTO_TEST_CASE(slot) {
    std::string const inp = "2022-07-10 14:56:32";
    std::string const fmt = "%Y-%m-%d %H:%M:%S";
    auto const d = cyng::make_date(inp, fmt);

    auto const r1 = smf::sml::to_index(d, smf::OBIS_PROFILE_15_MINUTE);
    BOOST_CHECK(r1.second);

    auto const d2 = smf::sml::from_index_to_date(r1.first, smf::OBIS_PROFILE_15_MINUTE);
    cyng::as_string(std::cout, d2, fmt);
    std::cout << std::endl;
    auto const s2 = cyng::as_string(d2, fmt);
    BOOST_REQUIRE_EQUAL(s2, "2022-07-10 15:00:00");

    // for (std::size_t idx = 0u; idx < 16u; ++idx) {
    //     auto const d = smf::sml::from_index_to_date(idx, smf::OBIS_PROFILE_15_MINUTE);
    //     cyng::as_string(std::cout, d, fmt);
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl;

    // for (std::size_t idx = 1841628u; idx < 1841628u + 16u; ++idx) {
    //     auto const d = smf::sml::from_index_to_date(idx, smf::OBIS_PROFILE_15_MINUTE);
    //     cyng::as_string(std::cout, d, fmt);
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl;

    // for (std::size_t idx = 0u; idx < 16u; ++idx) {
    //     auto const d = smf::sml::from_index_to_date(idx, smf::OBIS_PROFILE_60_MINUTE);
    //     cyng::as_string(std::cout, d, fmt);
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl;

    for (std::size_t idx = 0u; idx < 16u; ++idx) {
        auto const d = smf::sml::from_index_to_date(idx, smf::OBIS_PROFILE_24_HOUR);
        cyng::as_string(std::cout, d, fmt);
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
