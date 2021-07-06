#ifdef STAND_ALONE
#define BOOST_TEST_MODULE unit_test
#endif

#include <boost/test/unit_test.hpp>

#include <smf/sml/generator.h>
#include <smf/sml/msg.h>
#include <smf/sml/parser.h>
#include <smf/sml/reader.h>
#include <smf/sml/tokenizer.h>
#include <smf/sml/unpack.h>
#include <smf/sml/value.hpp>
#include <smf/sml/writer.hpp>

#include <cyng/io/ostream.h>
#include <cyng/parse/buffer.h>
#include <cyng/parse/string.h>

#include <fstream>
#include <iostream>

#ifdef _DEBUG
#include <cyng/io/hex_dump.hpp>
#include <iostream>
#include <sstream>
#endif

BOOST_AUTO_TEST_SUITE(sml_suite)

BOOST_AUTO_TEST_CASE(tokenizer) {
    std::ifstream ifile("..\\..\\unit_test\\assets\\smf--SML-202012T071425-power@solostec8bbb6264-bc4a4b92.sml", std::ios::binary);
    BOOST_CHECK(ifile.is_open());
    if (ifile.is_open()) {
        //	dont skip whitespaces
        ifile >> std::noskipws;
        smf::sml::tokenizer p([](smf::sml::sml_type type, std::size_t size, cyng::buffer_t data) {
            std::cout << smf::sml::get_name(type) << '[' << size << '/' << data.size() << ']' << std::endl;
        });
        p.read(std::istream_iterator<char>(ifile), std::istream_iterator<char>());
    }
}

BOOST_AUTO_TEST_CASE(parser) {
    std::ifstream ifile("..\\..\\unit_test\\assets\\smf--SML-202012T071425-power@solostec8bbb6264-bc4a4b92.sml", std::ios::binary);
    BOOST_CHECK(ifile.is_open());
    if (ifile.is_open()) {
        //	dont skip whitespaces
        ifile >> std::noskipws;
        smf::sml::parser p(
            [](std::string trx, std::uint8_t, std::uint8_t, smf::sml::msg_type type, cyng::tuple_t msg, std::uint16_t) {
                std::cout << "> " << smf::sml::get_name(type) << ": " << trx << ", " << msg << std::endl;
            });

        //	});
        p.read(std::istream_iterator<char>(ifile), std::istream_iterator<char>());
    }
}

BOOST_AUTO_TEST_CASE(login) {
    auto const inp = cyng::make_buffer({
        0x1b,
        0x1b,
        0x1b,
        0x1b,
        0x01,
        0x01,
        0x01,
        0x01,
        /* trailer */ 0x76,
        0x81,
        0x05,
        0x32,
        0x31,
        0x30,
        0x34,
        0x32, // ........ v..21042
        0x37,
        0x31,
        0x36,
        0x31,
        0x37,
        0x30,
        0x34,
        0x36,
        0x38,
        0x36,
        0x35,
        0x36,
        0x2d,
        0x31,
        0x62,
        0x00, // 71617046 8656-1b.
        0x62,
        0x00,
        0x72,
        0x63,
        0x01,
        0x00,
        0x77,
        0x01,
        0x07,
        0x00,
        0x50,
        0x56,
        0xc0,
        0x00,
        0x08,
        0x0f, // b.rc..w. ..PV....
        0x32,
        0x30,
        0x32,
        0x31,
        0x30,
        0x34,
        0x32,
        0x37,
        0x31,
        0x36,
        0x31,
        0x37,
        0x30,
        0x34,
        0x08,
        0x05, // 20210427 161704..
        0x00,
        0x15,
        0x3b,
        0x01,
        0xec,
        0x46,
        0x09,
        0x6f,
        0x70,
        0x65,
        0x72,
        0x61,
        0x74,
        0x6f,
        0x72,
        0x09, // ..;..F.o perator.
        0x6f,
        0x70,
        0x65,
        0x72,
        0x61,
        0x74,
        0x6f,
        0x72,
        0x01,
        0x63,
        0x8d,
        0x3b,
        0x00,
        0x76,
        0x81,
        0x05, // operator .c.;.v..
        0x32,
        0x31,
        0x30,
        0x34,
        0x32,
        0x37,
        0x31,
        0x36,
        0x31,
        0x37,
        0x30,
        0x34,
        0x36,
        0x38,
        0x36,
        0x35, // 21042716 17046865
        0x36,
        0x2d,
        0x32,
        0x62,
        0x01,
        0x62,
        0x00,
        0x72,
        0x63,
        0x05,
        0x00,
        0x75,
        0x08,
        0x05,
        0x00,
        0x15, // 6-2b.b.r c..u....
        0x3b,
        0x01,
        0xec,
        0x46,
        0x09,
        0x6f,
        0x70,
        0x65,
        0x72,
        0x61,
        0x74,
        0x6f,
        0x72,
        0x09,
        0x6f,
        0x70, // ;..F.ope rator.op
        0x65,
        0x72,
        0x61,
        0x74,
        0x6f,
        0x72,
        0x71,
        0x07,
        0x81,
        0x00,
        0x60,
        0x05,
        0x00,
        0x00,
        0x01,
        0x62, // eratorq. ..`....b
        0x8d,
        0x00,
        0x76,
        0x81,
        0x05,
        0x32,
        0x31,
        0x30,
        0x34,
        0x32,
        0x37,
        0x31,
        0x36,
        0x31,
        0x37,
        0x30, // ..v..210 42716170
        0x34,
        0x36,
        0x38,
        0x36,
        0x35,
        0x36,
        0x2d,
        0x33,
        0x62,
        0x02,
        0x62,
        0x00,
        0x72,
        0x63,
        0x05,
        0x00, // 468656-3 b.b.rc..
        0x75,
        0x08,
        0x05,
        0x00,
        0x15,
        0x3b,
        0x01,
        0xec,
        0x46,
        0x09,
        0x6f,
        0x70,
        0x65,
        0x72,
        0x61,
        0x74, // u....;.. F.operat
        0x6f,
        0x72,
        0x09,
        0x6f,
        0x70,
        0x65,
        0x72,
        0x61,
        0x74,
        0x6f,
        0x72,
        0x71,
        0x07,
        0x81,
        0x81,
        0xc7, // or.opera torq....
        0x82,
        0x01,
        0xff,
        0x01,
        0x63,
        0xb5,
        0x0f,
        0x00,
        0x76,
        0x81,
        0x05,
        0x32,
        0x31,
        0x30,
        0x34,
        0x32, // ....c... v..21042
        0x37,
        0x31,
        0x36,
        0x31,
        0x37,
        0x30,
        0x34,
        0x36,
        0x38,
        0x36,
        0x35,
        0x36,
        0x2d,
        0x34,
        0x62,
        0x00, // 71617046 8656-4b.
        0x62,
        0x00,
        0x72,
        0x63,
        0x02,
        0x00,
        0x71,
        0x01,
        0x63,
        0x37,
        0x58,
        0x00,
        /* post */ 0x1b,
        0x1b,
        0x1b,
        0x1b, // b.rc..q. c7X.....
        0x1a,
        0x00,
        0xf8,
        0xad //                                      ....
    });

    smf::sml::unpack p(
        [](std::string trx, std::uint8_t, std::uint8_t, smf::sml::msg_type type, cyng::tuple_t msg, std::uint16_t crc) {
            std::cout << "> " << smf::sml::get_name(type) << ": " << trx << ", " << msg << std::endl;
        });
    p.read(std::begin(inp), std::end(inp));
    BOOST_REQUIRE(p.get_parser().is_closed());

    /*
    ### Message ###
    76                                                SML_Message(Sequence):
      810532313034323731363137303436383635362D31      transactionId: 21042716170468656-1
      6200                                            groupNo: 0
      6200                                            abortOnError: 0
      72                                              messageBody(Choice):
            630100                                        messageBody: 256 => SML_PublicOpen_Req (0x00000100)
            77                                            SML_PublicOpen_Req(Sequence):
              01                                          codepage: not set
              07005056C00008                              clientId: 00 50 56 C0 00 08
              0F3230323130343237313631373034              reqFileId: 20210427161704
              080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46
              096F70657261746F72                          username: operator
              096F70657261746F72                          password: operator
              01                                          sml_Version: not set
      638D3B                                          crc16: 36155
      00                                              endOfSmlMsg: 00
    ### Message ###
    76                                                SML_Message(Sequence):
      810532313034323731363137303436383635362D32      transactionId: 21042716170468656-2
      6201                                            groupNo: 1
      6200                                            abortOnError: 0
      72                                              messageBody(Choice):
            630500                                        messageBody: 1280 => SML_GetProcParameter_Req (0x00000500)
            75                                            SML_GetProcParameter_Req(Sequence):
              080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46
              096F70657261746F72                          username: operator
              096F70657261746F72                          password: operator
              71                                          parameterTreePath(SequenceOf):
                    07810060050000                            path_Entry: 81 00 60 05 00 00
              01                                          attribute: not set
      628D                                            crc16: 141
      00                                              endOfSmlMsg: 00
    ### Message ###
    76                                                SML_Message(Sequence):
      810532313034323731363137303436383635362D33      transactionId: 21042716170468656-3
      6202                                            groupNo: 2
      6200                                            abortOnError: 0
      72                                              messageBody(Choice):
            630500                                        messageBody: 1280 => SML_GetProcParameter_Req (0x00000500)
            75                                            SML_GetProcParameter_Req(Sequence):
              080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46
              096F70657261746F72                          username: operator
              096F70657261746F72                          password: operator
              71                                          parameterTreePath(SequenceOf):
                    078181C78201FF                            path_Entry: 81 81 C7 82 01 FF
              01                                          attribute: not set
      63B50F                                          crc16: 46351
      00                                              endOfSmlMsg: 00
    ### Message ###
    76                                                SML_Message(Sequence):
      810532313034323731363137303436383635362D34      transactionId: 21042716170468656-4
      6200                                            groupNo: 0
      6200                                            abortOnError: 0
      72                                              messageBody(Choice):
            630200                                        messageBody: 512 => SML_PublicClose_Req (0x00000200)
            71                                            SML_PublicClose_Req(Sequence):
              01                                          globalSignature: not set
      633758                                          crc16: 14168
      00                                              endOfSmlMsg: 00
    */
}

BOOST_AUTO_TEST_CASE(value) {
    auto const tpl_01 = smf::sml::make_value(1);
    BOOST_REQUIRE_EQUAL(tpl_01.size(), 2);

    auto const tpl_02 = smf::sml::make_value("hello");
    BOOST_REQUIRE_EQUAL(tpl_02.size(), 2);

    const char s[6] = {'w', 'o', 'r', 'l', 'd', '!'};
    auto const tpl_03 = smf::sml::make_value(s);
    BOOST_REQUIRE_EQUAL(tpl_03.size(), 2);

    auto const key = cyng::make_aes_key<cyng::crypto::aes128_size>(cyng::hex_to_buffer("6E3272357538782F413F4428472B4B62"));
    auto const tpl_04 = smf::sml::make_value(key);
    BOOST_REQUIRE_EQUAL(tpl_04.size(), 2);
}

BOOST_AUTO_TEST_CASE(get_profile_list_response) {
    auto const inp = cyng::make_buffer({
        /* [0000] */ 0x1b,
        0x1b,
        0x1b,
        0x1b,
        0x01,
        0x01,
        0x01,
        0x01,
        0x76,
        0x07,
        0x34,
        0x33,
        0x36,
        0x38,
        0x35,
        0x38, // ........ v.436858
        /* [0010] */ 0x62,
        0x00,
        0x62,
        0x00,
        0x72,
        0x63,
        0x01,
        0x01,
        0x76,
        0x01,
        0x08,
        0x05,
        0x00,
        0x15,
        0x3b,
        0x01, // b.b.rc.. v.....;.
        /* [0020] */ 0xec,
        0x46,
        0x07,
        0x34,
        0x33,
        0x36,
        0x38,
        0x35,
        0x37,
        0x0a,
        0x01,
        0xe6,
        0x1e,
        0x57,
        0x14,
        0x06, // .F.43685 7....W..
        /* [0030] */ 0x21,
        0x36,
        0x03,
        0x01,
        0x01,
        0x63,
        0x59,
        0xb7,
        0x00, // !6...cY. .

        /* [0000] */ 0x76,
        0x07,
        0x34,
        0x33,
        0x36,
        0x38,
        0x35,
        0x39,
        0x62,
        0x00,
        0x62,
        0x00,
        0x72,
        0x63,
        0x04,
        0x01, // v.436859 b.b.rc..
        /* [0010] */ 0x79,
        0x0a,
        0x01,
        0xe6,
        0x1e,
        0x57,
        0x14,
        0x06,
        0x21,
        0x36,
        0x03,
        0x72,
        0x62,
        0x02,
        0x65,
        0x60, // y....W.. !6.rb.e`
        /* [0020] */ 0xa1,
        0x4f,
        0xd5,
        0x62,
        0x00,
        0x71,
        0x07,
        0x81,
        0x81,
        0xc7,
        0x86,
        0x11,
        0xff,
        0x72,
        0x62,
        0x01, // .O.b.q.. .....rb.
        /* [0030] */ 0x65,
        0x0a,
        0xe6,
        0xfe,
        0xd6,
        0x62,
        0x00,
        0x77,
        0x75,
        0x07,
        0x00,
        0x00,
        0x60,
        0x01,
        0x01,
        0xff, // e....b.w u...`...
        /* [0040] */ 0x62,
        0xff,
        0x52,
        0x00,
        0x09,
        0xe6,
        0x1e,
        0x57,
        0x14,
        0x06,
        0x21,
        0x36,
        0x03,
        0x01,
        0x75,
        0x07, // b.R....W ..!6..u.
        /* [0050] */ 0x00,
        0x00,
        0x61,
        0x61,
        0x00,
        0xff,
        0x62,
        0xff,
        0x52,
        0x00,
        0x62,
        0x00,
        0x01,
        0x75,
        0x07,
        0x07, // ..aa..b. R.b..u..
        /* [0060] */ 0x00,
        0x03,
        0x01,
        0x00,
        0x01,
        0x62,
        0x0d,
        0x52,
        0xfe,
        0x55,
        0x00,
        0x18,
        0xe7,
        0x5d,
        0x01,
        0x75, // .....b.R .U...].u
        /* [0070] */ 0x07,
        0x07,
        0x00,
        0x03,
        0x01,
        0x00,
        0xff,
        0x62,
        0x0d,
        0x52,
        0xfe,
        0x55,
        0x00,
        0x18,
        0xe7,
        0x5d, // .......b .R.U...]
        /* [0080] */ 0x01,
        0x75,
        0x07,
        0x81,
        0x81,
        0xc7,
        0x82,
        0x03,
        0xff,
        0x62,
        0xff,
        0x52,
        0x00,
        0x04,
        0x47,
        0x57, // .u...... .b.R..GW
        /* [0090] */ 0x46,
        0x01,
        0x75,
        0x07,
        0x01,
        0x00,
        0x00,
        0x09,
        0x0b,
        0x00,
        0x62,
        0x07,
        0x52,
        0x00,
        0x65,
        0x60, // F.u..... ..b.R.e`
        /* [00a0] */ 0xa1,
        0x4b,
        0xa7,
        0x01,
        0x75,
        0x07,
        0x81,
        0x06,
        0x2b,
        0x07,
        0x00,
        0x00,
        0x62,
        0xff,
        0x52,
        0x00, // .K..u... +...b.R.
        /* [00b0] */ 0x55,
        0xff,
        0xff,
        0xff,
        0xb1,
        0x01,
        0x01,
        0x01,
        0x63,
        0xee,
        0xb2,
        0x00, // U....... c...

        /* [0000] */ 0x76,
        0x07,
        0x34,
        0x33,
        0x36,
        0x38,
        0x35,
        0x39,
        0x62,
        0x00,
        0x62,
        0x00,
        0x72,
        0x63,
        0x04,
        0x01, // v.436859 b.b.rc..
        /* [0010] */ 0x79,
        0x0a,
        0x01,
        0xe6,
        0x1e,
        0x57,
        0x14,
        0x06,
        0x21,
        0x36,
        0x03,
        0x72,
        0x62,
        0x02,
        0x65,
        0x60, // y....W.. !6.rb.e`
        /* [0020] */ 0xa1,
        0x4f,
        0xd5,
        0x62,
        0x00,
        0x71,
        0x07,
        0x81,
        0x81,
        0xc7,
        0x86,
        0x11,
        0xff,
        0x72,
        0x62,
        0x01, // .O.b.q.. .....rb.
        /* [0030] */ 0x65,
        0x0a,
        0xe7,
        0x02,
        0x96,
        0x62,
        0x00,
        0x77,
        0x75,
        0x07,
        0x00,
        0x00,
        0x60,
        0x01,
        0x01,
        0xff, // e....b.w u...`...
        /* [0040] */ 0x62,
        0xff,
        0x52,
        0x00,
        0x09,
        0xe6,
        0x1e,
        0x57,
        0x14,
        0x06,
        0x21,
        0x36,
        0x03,
        0x01,
        0x75,
        0x07, // b.R....W ..!6..u.
        /* [0050] */ 0x00,
        0x00,
        0x61,
        0x61,
        0x00,
        0xff,
        0x62,
        0xff,
        0x52,
        0x00,
        0x62,
        0x00,
        0x01,
        0x75,
        0x07,
        0x07, // ..aa..b. R.b..u..
        /* [0060] */ 0x00,
        0x03,
        0x01,
        0x00,
        0x01,
        0x62,
        0x0d,
        0x52,
        0xfe,
        0x55,
        0x00,
        0x18,
        0xe7,
        0x5d,
        0x01,
        0x75, // .....b.R .U...].u
        /* [0070] */ 0x07,
        0x07,
        0x00,
        0x03,
        0x01,
        0x00,
        0xff,
        0x62,
        0x0d,
        0x52,
        0xfe,
        0x55,
        0x00,
        0x18,
        0xe7,
        0x5d, // .......b .R.U...]
        /* [0080] */ 0x01,
        0x75,
        0x07,
        0x81,
        0x81,
        0xc7,
        0x82,
        0x03,
        0xff,
        0x62,
        0xff,
        0x52,
        0x00,
        0x04,
        0x47,
        0x57, // .u...... .b.R..GW
        /* [0090] */ 0x46,
        0x01,
        0x75,
        0x07,
        0x01,
        0x00,
        0x00,
        0x09,
        0x0b,
        0x00,
        0x62,
        0x07,
        0x52,
        0x00,
        0x65,
        0x60, // F.u..... ..b.R.e`
        /* [00a0] */ 0xa1,
        0x4f,
        0x67,
        0x01,
        0x75,
        0x07,
        0x81,
        0x06,
        0x2b,
        0x07,
        0x00,
        0x00,
        0x62,
        0xff,
        0x52,
        0x00, // .Og.u... +...b.R.
        /* [00b0] */ 0x55,
        0xff,
        0xff,
        0xff,
        0xae,
        0x01,
        0x01,
        0x01,
        0x63,
        0x1b,
        0xd2,
        0x00 // U....... c...
    });

    smf::sml::unpack p(
        [](std::string trx, std::uint8_t, std::uint8_t, smf::sml::msg_type type, cyng::tuple_t msg, std::uint16_t crc) {
            std::cout << "> " << smf::sml::get_name(type) << ": " << trx << ", " << msg << std::endl;
            if (type == smf::sml::msg_type::GET_PROFILE_LIST_RESPONSE) {
                auto const r = smf::sml::read_get_profile_list_response(msg);
                for (auto const &ro : std::get<5>(r)) {
                    std::cout << ">> " << ro.first << ": " << ro.second << std::endl;
                }
            }
        });
    p.read(std::begin(inp), std::end(inp));
    // BOOST_REQUIRE(p.get_parser().is_closed());
}

BOOST_AUTO_TEST_CASE(serializer) {

    std::stringstream os;
    auto const size = cyng::io::serializer<std::uint32_t, cyng::io::SML>::write(os, 139777u);
    cyng::buffer_t r;
    r.resize(size);
    os.read(r.data(), size);
    BOOST_REQUIRE_EQUAL(r.size(), 4);
    BOOST_REQUIRE_EQUAL(r.at(0), 0x64);
    BOOST_REQUIRE_EQUAL(r.at(1), 0x02);
    BOOST_REQUIRE_EQUAL(r.at(2), 0x22);
    BOOST_REQUIRE_EQUAL(r.at(3), 0x01);
}

BOOST_AUTO_TEST_CASE(generator) {
    smf::sml::response_generator rgen;

    auto const file_id = cyng::hex_to_buffer("21070521492320490-1b");
    auto const client = cyng::hex_to_buffer("005056C00008");

    {
        smf::sml::messages_t msgs;
        msgs.push_back(rgen.public_open("21042716170468656-1", file_id, client, "0500153B01EC46"));
        auto const buffer = smf::sml::to_sml(msgs);

#ifdef _DEBUG
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, std::begin(buffer), std::end(buffer));
            auto const dmp = ss.str();
            std::cerr << "response " << buffer.size() << " bytes:\n" << dmp;
        }
#endif
        BOOST_REQUIRE_EQUAL(buffer.size(), 0x4c);
        BOOST_REQUIRE_EQUAL(buffer.at(0x38), 0);
    }
    auto const server = cyng::hex_to_buffer("005056C00008");
    {
        auto const path = cyng::make_obis_path(cyng::hex_to_buffer("8148170700FF"));
        smf::sml::messages_t msgs;
        msgs.push_back(rgen.get_proc_parameter(
            "21042716170468656-1", server, path, smf::sml::tree_param(path.front(), smf::sml::make_value(1234))));
        auto const buffer = smf::sml::to_sml(msgs);
#ifdef _DEBUG
        {
            std::stringstream ss;
            cyng::io::hex_dump<8> hd;
            hd(ss, std::begin(buffer), std::end(buffer));
            auto const dmp = ss.str();
            std::cerr << "response " << buffer.size() << " bytes:\n" << dmp;
        }
#endif
    }
}
BOOST_AUTO_TEST_SUITE_END()
