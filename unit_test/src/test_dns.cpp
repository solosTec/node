#ifdef STAND_ALONE
#define BOOST_TEST_MODULE unit_test
#endif

#include <boost/test/unit_test.hpp>

#include <smf/dns/parser.h>

#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/obj/factory.hpp>
#include <cyng/obj/value_cast.hpp>

#include <chrono>
#include <iostream>

BOOST_AUTO_TEST_SUITE(dns_suite)

BOOST_AUTO_TEST_CASE(msg) {
    //{
    //    constexpr std::uint8_t mask{0b0000'0001};
    //    std::cout << +mask << std::endl;  // 1
    //}
    //{
    //    constexpr std::uint8_t mask{0b1000'0000};
    //    std::cout << +mask << std::endl; // 128
    //}

    // [0000]  f3 94 01 20 00 01 00 00  00 00 00 01 08 6e 6f 72  ... .... .....nor
    // [0010]  6d 61 6e 64 79 03 63 64  6e 07 6d 6f 7a 69 6c 6c  mandy.cd n.mozill
    // [0020]  61 03 6e 65 74 00 00 01  00 01 00 00 29 04 b0 00  a.net... ....)...
    // [0030]  00 00 00 00 00
    auto const inp = cyng::make_buffer({

        0xf3, 0x94, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x6e, 0x6f, 0x72, 0x6d, 0x61,
        0x6e, 0x64, 0x79, 0x03, 0x63, 0x64, 0x6e, 0x07, 0x6d, 0x6f, 0x7a, 0x69, 0x6c, 0x6c, 0x61, 0x03, 0x6e, 0x65,
        0x74, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x29, 0x04, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

    });

    smf::dns::parser p;
    p.read(inp.begin(), inp.end());

    std::cout << "id: " << p.get_msg().get_id() << std::endl;
    using smf::operator<<;
    std::cout << p.get_msg() << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
