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

BOOST_AUTO_TEST_CASE(query) {
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
    // auto const inp = cyng::make_buffer({

    //    0xf3, 0x94, 0x01, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x6e, 0x6f, 0x72, 0x6d, 0x61,
    //    0x6e, 0x64, 0x79, 0x03, 0x63, 0x64, 0x6e, 0x07, 0x6d, 0x6f, 0x7a, 0x69, 0x6c, 0x6c, 0x61, 0x03, 0x6e, 0x65,
    //    0x74, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x29, 0x04, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

    //});

    //  https://mislove.org/teaching/cs4700/spring11/handouts/project1-primer.pdf
    auto const inp = cyng::make_buffer({

        //    01    02    03    04    05    06    07    08    09    10    11    12    13    14    15
        0xdb, 0x42, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x77, 0x77, 0x77, //
        0x0c, 0x6e, 0x6f, 0x72, 0x74, 0x68, 0x65, 0x61, 0x73, 0x74, 0x65, 0x72, 0x6e, 0x03, 0x65, 0x64, //
        0x75, 0x00, 0x00, 0x01, 0x00, 0x01                                                              //

    });

    //
    //  result:
    //  ID      : 0xdb42
    //  QR      : 0 (Query)
    //  OPCODE  : 0 (standard)
    //  TC      : 0 (not truncated)
    //  RD      : 1 (recursion requested)
    //  RA      : 0 (not meaning for query)
    //  Z       : 0 (reserved)
    //  RCODE   : 0 (not meaning for query)
    //  QD      : 1 (one question)
    //  AN      : 0 (no answer)
    //  NS      : 0 (no records follow)
    //  AR      : 0 (no additional records follow)
    //

    smf::dns::parser p;
    p.read(inp.begin(), inp.end());

    // std::cout << "id: " << p.get_msg().get_id() << std::endl;
    using smf::operator<<;
    std::cout << p.get_msg() << std::endl;
    BOOST_REQUIRE_EQUAL(p.get_msg().get_id(), 0xdb42);
    BOOST_REQUIRE(p.get_msg().is_query());                               // QR
    BOOST_REQUIRE(p.get_msg().get_opcode() == smf::dns::op_code::QUERY); // OPCODE
    BOOST_REQUIRE(!p.get_msg().is_truncated());                          // TC
    BOOST_REQUIRE_EQUAL(p.get_msg().get_qd_count(), 1);                  // QD
    BOOST_REQUIRE_EQUAL(p.get_msg().get_an_count(), 0);                  // AN
    BOOST_REQUIRE_EQUAL(p.get_msg().get_ns_count(), 0);                  // NS
    BOOST_REQUIRE_EQUAL(p.get_msg().get_ar_count(), 0);                  // AR
}

BOOST_AUTO_TEST_CASE(response) {

    //  https://mislove.org/teaching/cs4700/spring11/handouts/project1-primer.pdf
    auto const inp = cyng::make_buffer({

        //    01    02    03    04    05    06    07    08    09    10    11    12    13    14    15
        0xdb, 0x42, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0x77, 0x77, 0x77, //
        0x0c, 0x6e, 0x6f, 0x72, 0x74, 0x68, 0x65, 0x61, 0x73, 0x74, 0x65, 0x72, 0x6e, 0x03, 0x65, 0x64, //
        0x75, 0x00, 0x00, 0x01, 0x00, 0x01, 0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x58, //
        0x00, 0x04, 0x9b, 0x21, 0x11, 0x44                                                              //

    });

    //
    //  result:
    //  ID      : 0xdb42
    //  Flags   : 0x8180
    //  QR      : 1 (Reponse)
    //  OPCODE  : 0 (standard)
    //  TC      : 0 (not truncated)
    //  RD      : 1 (recursion requested)
    //  RA      : 1 (server can do recursion)
    //  Z       : 0 (reserved)
    //  RCODE   : 0 (not meaning for query)
    //  QD      : 1 (one question)
    //  AN      : 0 (no answer)
    //  NS      : 0 (no records follow)
    //  AR      : 0 (no additional records follow)
    //

    smf::dns::parser p;
    p.read(inp.begin(), inp.end());

    // std::cout << "id: " << p.get_msg().get_id() << std::endl;
    using smf::operator<<;
    std::cout << p.get_msg() << std::endl;
    BOOST_REQUIRE_EQUAL(p.get_msg().get_id(), 0xdb42);
    BOOST_REQUIRE(!p.get_msg().is_query());                              // QR
    BOOST_REQUIRE(p.get_msg().get_opcode() == smf::dns::op_code::QUERY); // OPCODE
    BOOST_REQUIRE(!p.get_msg().is_truncated());                          // TC
    BOOST_REQUIRE_EQUAL(p.get_msg().get_qd_count(), 1);                  // QD
    BOOST_REQUIRE_EQUAL(p.get_msg().get_an_count(), 1);                  // AN
    BOOST_REQUIRE_EQUAL(p.get_msg().get_ns_count(), 0);                  // NS
    BOOST_REQUIRE_EQUAL(p.get_msg().get_ar_count(), 0);                  // AR
}

BOOST_AUTO_TEST_SUITE_END()
