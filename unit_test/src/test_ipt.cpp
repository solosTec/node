#ifdef STAND_ALONE
#define BOOST_TEST_MODULE unit_test
#endif

#include <boost/test/unit_test.hpp>

#include <smf/ipt/parser.h>
#include <smf/ipt/serializer.h>
#include <smf/ipt/transpiler.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/ostream.h>

#include <fstream>
#include <iostream>

BOOST_AUTO_TEST_SUITE(ipt_suite)

void foo(cyng::buffer_t b) {
    std::cout << b.size() << std::endl;
    b.clear();
}

BOOST_AUTO_TEST_CASE(scrambler) {
    auto b = cyng::make_buffer("hello, world! 1234567890");
    foo(std::move(b));
    BOOST_REQUIRE(b.empty());

    auto b2 = cyng::make_buffer("hello, world! 1234567890");
    auto b3 = std::move(b2);
    BOOST_REQUIRE(b2.empty());

    BOOST_REQUIRE_EQUAL(2 ^ 3, 3 ^ 2);
    BOOST_REQUIRE_EQUAL(120 ^ 376, 376 ^ 120);

    auto const sk = smf::ipt::gen_random_sk();

    smf::ipt::parser p(
        sk,
        [](smf::ipt::header const &, cyng::buffer_t &&) {

        },
        [](cyng::buffer_t &&data) {
            std::string str(data.begin(), data.end());
            std::cout << str << std::endl;
        });

    smf::ipt::serializer s(sk);

    auto const data = s.scramble(cyng::make_buffer("hello, world! 1234567890"));
    auto const descrambled = p.read(data.begin(), data.end());

    BOOST_REQUIRE_EQUAL(data.size(), descrambled.size());
    if (data.size() == descrambled.size()) {
        for (std::size_t idx = 0; idx < data.size(); ++idx) {
            BOOST_REQUIRE_EQUAL(data.at(idx), descrambled.at(idx));
        }
    }
}

BOOST_AUTO_TEST_CASE(transpiler) {

    {
        auto body = cyng::make_buffer({0x03, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0xc1, 0x00, 0x3d, 0x00, 0x00, 0x00, 0x1b,
                                       0x1b, 0x1b, 0x1b, 0x01, 0x01, 0x01, 0x01, 0x76, 0x09, 0x35, 0x33, 0x32, 0x36, 0x31, 0x31,
                                       0x30, 0x32, 0x62, 0x00, 0x62, 0x00, 0x72, 0x63, 0x01, 0x01, 0x76, 0x01, 0x08, 0x05, 0x00,
                                       0x15, 0x3b, 0x02, 0x29, 0x80, 0x09, 0x35, 0x33, 0x32, 0x36, 0x31, 0x31, 0x30, 0x31, 0x0a,
                                       0x01, 0xe6, 0x1e, 0x57, 0x14, 0x06, 0x21, 0x36, 0x03, 0x01, 0x01, 0x63, 0x03, 0x96, 0x00});

        auto [channel, source, status, block, data] = smf::ipt::tp_req_pushdata_transfer(std::move(body));
        BOOST_REQUIRE_EQUAL(channel, 3);
        BOOST_REQUIRE_EQUAL(source, 6);
        BOOST_REQUIRE_EQUAL(status, 193); //  0xc1
        BOOST_REQUIRE_EQUAL(block, 0);
        BOOST_REQUIRE_EQUAL(data.size(), 61);
        BOOST_REQUIRE_EQUAL(data.at(0), 27);
    }
    {
        auto body = cyng::make_buffer({100, 108, 109, 115, 64, 115, 116, 111, 114, 101, 0, 0, 0, 0, 0, 30, 0});
        auto const [target, account, msisdn, version, id, timeout] = smf::ipt::tp_req_open_push_channel(std::move(body));
        BOOST_REQUIRE_EQUAL(target, "dlms@store");
    }
}

BOOST_AUTO_TEST_CASE(parser) {

    smf::ipt::serializer s(smf::ipt::scramble_key::default_scramble_key_);
    smf::ipt::parser p(
        smf::ipt::scramble_key::default_scramble_key_,
        [](smf::ipt::header const &h, cyng::buffer_t &&body) -> void {
            std::cout << "cmd: " << smf::ipt::command_name(h.command_) << std::endl;
        },
        [](cyng::buffer_t &&) -> void { std::cout << "data" << std::endl; });

    auto r1 = s.req_login_public("name", "pwd");
    auto r2 = s.req_open_push_channel("target-1", "account-1", "number-1", "version-1", "id-1", 1);
    auto r3 = s.req_open_push_channel("target-2", "account-2", "number-2", "version-2", "id-2", 2);

    cyng::buffer_t merged;
    merged.insert(merged.end(), r1.begin(), r1.end());
    merged.insert(merged.end(), r2.first.begin(), r2.first.end());
    merged.insert(merged.end(), r3.first.begin(), r3.first.end());

    p.read(merged.begin(), merged.end());

    std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE(serializer) {
    auto const sk = smf::ipt::scramble_key::default_scramble_key_;
    // auto const sk = smf::ipt::gen_random_sk();
    smf::ipt::serializer s(sk);
    s.set_sk(sk);

    auto const r = s.escape_data(cyng::make_buffer({0xcb, 0x16, 0x1b, 0x62, 0x2d, 0xb3, 0xc9, 0x1b}));
    //  cb161b1b622db3c9
    auto const cmp = cyng::io::to_hex(r);
    std::cout << cmp << std::endl;
    BOOST_REQUIRE_EQUAL(cmp, "ca14181f672bb4c1121b");
    // BOOST_REQUIRE_EQUAL(cmp, "ca141f18672bb4c1");
    //     BOOST_REQUIRE_EQUAL(cmp, "cb161b1b622db3c9");

    // auto const r = s.escape_data(cyng::make_buffer({0x2a, 0x1b, 0x1b, 0x20, 0x21, 0x22, 0x1b}));
    ////  2a1b1b1b1b2021221b1b
    // auto const cmp = cyng::io::to_hex(r);
    // std::cout << cmp << std::endl;
    // BOOST_REQUIRE_EQUAL(cmp, "2a1b1b1b1b2021221b1b");
}

BOOST_AUTO_TEST_SUITE_END()
