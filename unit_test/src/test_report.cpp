#ifdef STAND_ALONE
#define BOOST_TEST_MODULE unit_test
#endif

#include <boost/test/unit_test.hpp>

#include <cyng/sys/clock.h>
#include <smf/report/utility.h>

#ifdef _DEBUG
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>
#endif

BOOST_AUTO_TEST_SUITE(report_suite)

BOOST_AUTO_TEST_CASE(offset) {

    // auto const now = std::chrono::system_clock::now();
    //
    //   generate a time stamp
    //
    std::tm tm = {};
    std::istringstream ss("2022-Oktober-21 23:12:34");
    ss.imbue(std::locale("de_DE.utf-8"));
    ss >> std::get_time(&tm, "%Y-%b-%d %H:%M:%S");

    auto const t = std::mktime(&tm);
    auto tp = std::chrono::system_clock::from_time_t(t);

    for (auto idx = 0u; idx < 14; ++idx) {
        auto const delta = cyng::sys::delta_utc(tp);
        // cyng::sys::to_string(tp, "%F %T%z");
        std::cout << cyng::sys::to_string(tp, "%F %T%z") << ", delta: " << delta << std::endl;
        // auto const adj = smf::make_tz_offset(tp);
        // std::cout << adj.duration() << ", " << cyng::sys::to_string(adj.local_time(), "%F %T%z") << std::endl;
        tp += std::chrono::hours(24);
    }
}

BOOST_AUTO_TEST_SUITE_END()
