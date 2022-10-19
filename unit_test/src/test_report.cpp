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
    // std::tm tm = {};
    // std::istringstream ss("2022-Oktober-21 23:12:34");
    // ss.imbue(std::locale("de_DE.utf-8"));
    //// std::istringstream ss("2022-Oct-21 23:12:34");
    //// ss.imbue(std::locale("en_US.UTF-8"));
    // ss >> std::get_time(&tm, "%Y-%b-%d %H:%M:%S");
    // tm.tm_isdst = 1; //  DST is in effect

    // auto const t = std::mktime(&tm);
    // auto tp = std::chrono::system_clock::from_time_t(t);

    std::string const inp = "2022-10-21 23:12:34";
    std::string const fmt = "%Y-%m-%d %H:%M:%S";
    auto tp = cyng::sys::to_time_point(inp, fmt);

    //   2022-10-22 00:12:34+0200 - de_DE.utf-8
    std::cout << "start time: " << cyng::sys::to_string_utc(tp, "%F %T%z (UTC), ") << cyng::sys::to_string(tp, "%F %T%z (local)")
              << std::endl;

    for (auto idx = 0u; idx < 14; ++idx) {
        // auto const delta = cyng::sys::delta_utc(tp);
        //  cyng::sys::to_string(tp, "%F %T%z");
        //  std::cout << cyng::sys::to_string(tp, "%F %T%z") << ", delta: " << delta << std::endl;
        auto const adj = smf::make_tz_offset(tp);
        // std::cout << adj.duration() << ", " << cyng::sys::to_string(adj.local_time(), "%F %T%z") << std::endl;
        std::cout << adj << std::endl;
        tp += std::chrono::hours(24);
    }

    // template <typename D> using tz_offset_t = tz_offset<typename D::rep, typename D::period>;
    typename smf::tz_offset_t<std::chrono::minutes>::type tz;
}

BOOST_AUTO_TEST_SUITE_END()
