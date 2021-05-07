#ifdef STAND_ALONE
#   define BOOST_TEST_MODULE unit_test
#endif

#include <boost/test/unit_test.hpp>

#include <smf/mbus/server_id.h>
#include <smf/mbus/radio/parser.h>
#include <smf/mbus/radio/header.h>
#include <smf/mbus/radio/decode.h>
#include <smf/mbus/reader.h>
#include <smf/mbus/bcd.hpp>
#include <smf/sml/unpack.h>

#include <cyng/io/ostream.h>
#include <cyng/parse/string.h>
#include <cyng/obj/intrinsics/buffer.h>

#include <iostream>
#include <fstream>


BOOST_AUTO_TEST_SUITE(mbus_suite)

BOOST_AUTO_TEST_CASE(id)
{
	//	Example: 0x3105c => 96072000
	auto const m1 = smf::to_meter_id(0x3105c);
	BOOST_REQUIRE_EQUAL(m1.size(), 4);
	BOOST_REQUIRE_EQUAL(m1.at(0) & 0xFF, 0x96);
	BOOST_REQUIRE_EQUAL(m1.at(1) & 0xFF, 0x07);
	BOOST_REQUIRE_EQUAL(m1.at(2) & 0xFF, 0x20);
	BOOST_REQUIRE_EQUAL(m1.at(3) & 0xFF, 0x00);
	//s.cpp(22) : fatal error : in "mbus_suite/id" : critical check m1.at(0) == 0x96 has failed[0xffffff96 != 0x96]

	//	 "10320047" = > [10, 32, 00, 47]dec
	auto const m2 = smf::to_meter_id("10320047");
	BOOST_REQUIRE_EQUAL(m2.size(), 4);
	BOOST_REQUIRE_EQUAL(m2.at(0) & 0xFF, 0x0a);
	BOOST_REQUIRE_EQUAL(m2.at(1) & 0xFF, 0x20);
	BOOST_REQUIRE_EQUAL(m2.at(2) & 0xFF, 0x00);
	BOOST_REQUIRE_EQUAL(m2.at(3) & 0xFF, 0x2f);

	auto const s1 = smf::srv_id_to_str(smf::srv_id_t({ 0x02, 0xe6, 0x1e, 0x03, 0x19, 0x77, 0x15, 0x3c, 0x07 }));
	BOOST_REQUIRE_EQUAL(s1, "02-e61e-03197715-3c-07");
}

BOOST_AUTO_TEST_CASE(parser)
{

	//	SML data
	//	[0000]  ce 44 a8 15 74 31 45 04  01 02 7f 46 00 c0 05 57  .D..t1E. ...F...W
	//	[0010]  35 f1 07 c8 b4 a7 67 21  17 90 97 fa bf cd 22 92  5.....g! ......".
	//	[0020]  ee c5 15 c5 7b 73 8a e3  29 7f 52 9c b1 ea d9 a4  ....{s.. ).R.....
	//	[0030]  1c ed c9 ab 75 73 4f 8a  3b 4b 42 8b e8 9d 40 02  ....usO. ;KB...@.
	//	[0040]  95 31 b3 0b 32 51 bf 7d  4c c0 da 37 79 77 1b 59  .1..2Q.} L..7yw.Y
	//	[0050]  a0 ac 3c 26 81 28 91 95  75 c3 53 68 89 3a e3 e7  ..<&.(.. u.Sh.:..
	//	[0060]  67 fe 6c cd 15 f8 02 3e  28 5b 43 8e 1a ca 5d bf  g.l....> ([C...].
	//	[0070]  0f b1 71 63 5e 00 95 95  39 d5 44 93 96 22 44 a1  ..qc^... 9.D.."D.
	//	[0080]  c4 9e a0 c6 5e 98 c8 8f  1c a7 cc 19 95 6c 88 c2  ....^... .....l..
	//	[0090]  dd 66 87 9a a8 6a 27 86  5a 11 54 dc 9b 7c d5 8f  .f...j'. Z.T..|..
	//	[00a0]  b0 17 fc 11 aa 6e a0 6e  c7 80 42 b4 b0 43 7b 58  .....n.n ..B..C{X
	//	[00b0]  29 15 30 b1 cc 35 7a 97  b4 6e 7e 41 02 10 b1 50  ).0..5z. .n~A...P
	//	[00c0]  c4 64 80 3d 6b d6 be c0  b1 45 1f 90 cc c2 cf     .d.=k... .E.....
	auto const inp = cyng::make_buffer({
		0xce, 0x44, 0xa8, 0x15, 0x74, 0x31, 0x45, 0x04, 0x01, 0x02, 0x7f, /* header complete */ 0x46, 0x00, 0xc0, 0x05, /* data */ 0x57,
		0x35, 0xf1, 0x07, 0xc8, 0xb4, 0xa7, 0x67, 0x21, 0x17, 0x90, 0x97, 0xfa, 0xbf, 0xcd, 0x22, 0x92,
		0xee, 0xc5, 0x15, 0xc5, 0x7b, 0x73, 0x8a, 0xe3, 0x29, 0x7f, 0x52, 0x9c, 0xb1, 0xea, 0xd9, 0xa4,
		0x1c, 0xed, 0xc9, 0xab, 0x75, 0x73, 0x4f, 0x8a, 0x3b, 0x4b, 0x42, 0x8b, 0xe8, 0x9d, 0x40, 0x02,
		0x95, 0x31, 0xb3, 0x0b, 0x32, 0x51, 0xbf, 0x7d, 0x4c, 0xc0, 0xda, 0x37, 0x79, 0x77, 0x1b, 0x59,
		0xa0, 0xac, 0x3c, 0x26, 0x81, 0x28, 0x91, 0x95, 0x75, 0xc3, 0x53, 0x68, 0x89, 0x3a, 0xe3, 0xe7,
		0x67, 0xfe, 0x6c, 0xcd, 0x15, 0xf8, 0x02, 0x3e, 0x28, 0x5b, 0x43, 0x8e, 0x1a, 0xca, 0x5d, 0xbf,
		0x0f, 0xb1, 0x71, 0x63, 0x5e, 0x00, 0x95, 0x95, 0x39, 0xd5, 0x44, 0x93, 0x96, 0x22, 0x44, 0xa1,
		0xc4, 0x9e, 0xa0, 0xc6, 0x5e, 0x98, 0xc8, 0x8f, 0x1c, 0xa7, 0xcc, 0x19, 0x95, 0x6c, 0x88, 0xc2,
		0xdd, 0x66, 0x87, 0x9a, 0xa8, 0x6a, 0x27, 0x86, 0x5a, 0x11, 0x54, 0xdc, 0x9b, 0x7c, 0xd5, 0x8f,
		0xb0, 0x17, 0xfc, 0x11, 0xaa, 0x6e, 0xa0, 0x6e, 0xc7, 0x80, 0x42, 0xb4, 0xb0, 0x43, 0x7b, 0x58,
		0x29, 0x15, 0x30, 0xb1, 0xcc, 0x35, 0x7a, 0x97, 0xb4, 0x6e, 0x7e, 0x41, 0x02, 0x10, 0xb1, 0x50,
		0xc4, 0x64, 0x80, 0x3d, 0x6b, 0xd6, 0xbe, 0xc0, 0xb1, 0x45, 0x1f, 0x90, 0xcc, 0xc2, 0xcf

		});

	smf::mbus::radio::parser p([&](smf::mbus::radio::header const& h, smf::mbus::radio::tpl const& t, cyng::buffer_t const& payload) {
		std::cout << smf::mbus::to_str(h) << std::endl;

		auto mc = smf::get_manufacturer_code(h.get_server_id());
		BOOST_REQUIRE_EQUAL(mc.first, static_cast<char>(0x0a8));
		BOOST_REQUIRE_EQUAL(mc.second, static_cast<char>(0x015));

		auto const aes = cyng::to_aes_key<cyng::crypto::aes128_size>("23A84B07EBCBAF948895DF0E9133520D");

		auto const res = smf::mbus::radio::decode(h.get_server_id()
			, t.get_access_no()
			, aes
			, payload);

		BOOST_REQUIRE_EQUAL(res.size(), 0xc0);
		BOOST_REQUIRE_EQUAL(res.at(0), 0x2f);
		BOOST_REQUIRE_EQUAL(res.at(1), 0x2f);

		//
		//	read SML data
		// 
		smf::sml::unpack p([](std::string trx, std::uint8_t, std::uint8_t, smf::sml::msg_type type, cyng::tuple_t msg, std::uint16_t crc) {

			std::cout << "> " << smf::sml::get_name(type) << ": " << trx << ", " << msg << std::endl;

			});
		p.read(res.begin() + 2, res.end());

		auto const clone = smf::mbus::radio::restore_data(h, t, payload);
		BOOST_REQUIRE_EQUAL(clone.size(), inp.size());
		for (std::size_t idx = 0; idx < inp.size(); ++idx) {
			BOOST_REQUIRE_EQUAL(clone.at(idx), inp.at(idx));
		}


		});
	p.read(std::begin(inp), std::end(inp));

	//	long header
	auto const inp_long = cyng::make_buffer({
		0x36, 0x44, 0xe6, 0x1e, 0x79, 0x42, 0x68, 0x00, 0x02, 0x0e, 0x72, /* header complete */ 0x57, 0x14, 0x06, 0x21, 0xe6,
		0x1e, 0x36, 0x03, 0xf3, 0x00, 0x20, 0x65, /* secondary complete */ 0xd4, 0xfc, 0xa9, 0xb9, 0x37, 0x81, 0x3f, 0xf1, 0x45,
		0xf0, 0x4c, 0x61, 0x1e, 0x65, 0x13, 0x43, 0x69, 0x60, 0x69, 0x43, 0x08, 0x86, 0x1c, 0xbc, 0x98,
		0x2d, 0xb5, 0x4a, 0xbb, 0x76, 0xb3, 0xa3 });

	smf::mbus::radio::parser p_long([&](smf::mbus::radio::header const& h, smf::mbus::radio::tpl const& t, cyng::buffer_t const& payload) {
		std::cout << smf::mbus::to_str(h) << std::endl;
		auto const sec = t.get_secondary_address();
		std::cout << smf::srv_id_to_str(sec) << std::endl;

		BOOST_REQUIRE_EQUAL(smf::srv_id_to_str(sec), "01-e61e-57140621-36-03");

		auto const aes = cyng::to_aes_key<cyng::crypto::aes128_size>("6140B8C066EDDE3773EDF7F8007A45AB");

		auto const res = smf::mbus::radio::decode(h.get_server_id()
			, t.get_access_no()
			, aes
			, payload);

		BOOST_REQUIRE_EQUAL(res.size(), 0x20);
		BOOST_REQUIRE_EQUAL(res.at(0), 0x2f);
		BOOST_REQUIRE_EQUAL(res.at(1), 0x2f);

		//smf::mbus::read(res, 2);

		auto const clone = smf::mbus::radio::restore_data(h, t, payload);
		BOOST_REQUIRE_EQUAL(clone.size(), inp_long.size());
		for (std::size_t idx = 0; idx < inp_long.size(); ++idx) {
			BOOST_REQUIRE_EQUAL(clone.at(idx), inp_long.at(idx));
		}


		});
	p_long.read(std::begin(inp_long), std::end(inp_long));

	auto const inp_short = cyng::make_buffer({
		0x2e,	//	length (46 bytes)
		0x44,	//	C-field (send/no reply)
		0x93,	//	manufacturer
		0x15,
		0x78,	//	address
		0x56,
		0x34,
		0x12,
		0x33,	//	version
		0x03,	//	device type (gas)
		0x7a,	//	CI (short header)
		0x2a,	//	access no
		0x00,	//	status
		0x20,	//	config
		0x25,
/*18*/	0x59,	//	- 0x2f, AES verify
/*19*/	0x23,	//	- 0x2f, AES verify
/*20*/	0xc9,	//	- 0x0c, DIF (8 digit BCD
/*21*/	0x5a,	//	- 0x14, VIF (Volume 0,01 cubic meter) 
/*22*/	0xaa,	//	- 0x27, Value LSB 

/*23*/	0x26,	//	- 
/*24*/	0xd1,	//	- 
/*25*/	0xb2,	//	- 
		0xe7,	//	- DIF (Time at readout; Type F)
		0x49,	//	- 
		0x3b,	//	- 
		0x01,	//	- 
		0x3e,	//	- 
		0xc4,	//	- 
		0xa6,	//	- DIF (2 byte integer)
		0xf6,	//	- 
		0xd3,	//	- 
		0x52,	//	- 
		0x9b,	//
/*39*/	0x52,	//	- 0x2f, AES fill bytes
		0x0e,	//	- 0x2f, AES fill bytes
		0xdf,	//	- 0x2f, AES fill bytes
		0xf0,	//	- 0x2f, AES fill bytes
		0xea,	//	- 0x2f, AES fill bytes
		0x6d,	//	- 0x2f, AES fill bytes
		0xef,	//	- 0x2f, AES fill bytes
		0xc9,	//	- 0x2f, AES fill bytes
		0x9d,	//	- 0x2f, AES fill bytes
		0x6d,	//	- 0x2f, AES fill bytes
		0x69,	//	- 0x2f, AES fill bytes
		0xeb,	//	- 0x2f, AES fill bytes
		0xf3,	//	- 0x2f, AES fill bytes
		});

	smf::mbus::radio::parser p_short([&](smf::mbus::radio::header const& h, smf::mbus::radio::tpl const& t, cyng::buffer_t const& payload) {
		std::cout << smf::mbus::to_str(h) << std::endl;

		auto mc = smf::get_manufacturer_code(h.get_server_id());
		BOOST_REQUIRE_EQUAL(mc.first, static_cast<char>(0x93));
		BOOST_REQUIRE_EQUAL(mc.second, static_cast<char>(0x15));

		auto const aes = cyng::to_aes_key<cyng::crypto::aes128_size>("0102030405060708090A0B0C0D0E0F11");

		auto const res = smf::mbus::radio::decode(h.get_server_id()
			, t.get_access_no()
			, aes
			, payload);

		BOOST_REQUIRE_EQUAL(res.size(), h.effective_payload_size());
		BOOST_REQUIRE_EQUAL(res.at(0), 0x2f);
		BOOST_REQUIRE_EQUAL(res.at(1), 0x2f);

		//
		//	clear dummy bytes
		//

		std::size_t offset = 2;
		while (offset < res.size()) {
			offset = smf::mbus::read(res, offset);
		}

		});
	p_short.read(std::begin(inp_short), std::end(inp_short));

}

BOOST_AUTO_TEST_CASE(dif)
{
	smf::mbus::dif d1(static_cast<std::uint8_t>(12u));
	BOOST_REQUIRE(d1.get_data_field_code() == smf::mbus::data_field_code::DFC_8_DIGIT_BCD);
	BOOST_REQUIRE(d1.get_function_field_code() == smf::mbus::function_field_code::INSTANT);
	BOOST_REQUIRE(!d1.is_extended());

	smf::mbus::dif d2(static_cast<std::uint8_t>(4u));
	BOOST_REQUIRE(d2.get_data_field_code() == smf::mbus::data_field_code::DFC_32_BIT_INT);	//	(Time at readout; Type F)
	BOOST_REQUIRE(d2.get_function_field_code() == smf::mbus::function_field_code::INSTANT);
	BOOST_REQUIRE(!d2.is_extended());

	smf::mbus::dif d3(static_cast<std::uint8_t>(2u));
	BOOST_REQUIRE(d3.get_data_field_code() == smf::mbus::data_field_code::DFC_16_BIT_INT);
	BOOST_REQUIRE(d3.get_function_field_code() == smf::mbus::function_field_code::INSTANT);
	BOOST_REQUIRE(!d3.is_extended());

	smf::mbus::dif d4(static_cast<std::uint8_t>(0x4c));
	BOOST_REQUIRE(d4.get_data_field_code() == smf::mbus::data_field_code::DFC_8_DIGIT_BCD);
	BOOST_REQUIRE(d4.get_function_field_code() == smf::mbus::function_field_code::INSTANT);
	BOOST_REQUIRE(d4.is_storage());
	BOOST_REQUIRE(!d4.is_extended());

	smf::mbus::dif d5(static_cast<std::uint8_t>(0x82));
	BOOST_REQUIRE(d5.get_data_field_code() == smf::mbus::data_field_code::DFC_16_BIT_INT);
	BOOST_REQUIRE(d5.get_function_field_code() == smf::mbus::function_field_code::INSTANT);
	BOOST_REQUIRE(!d5.is_storage());
	BOOST_REQUIRE(d5.is_extended());

	smf::mbus::dif d6(static_cast<std::uint8_t>(0x8C));
	BOOST_REQUIRE(d6.get_data_field_code() == smf::mbus::data_field_code::DFC_8_DIGIT_BCD);
	BOOST_REQUIRE(d6.get_function_field_code() == smf::mbus::function_field_code::INSTANT);
	BOOST_REQUIRE(!d6.is_storage());
	BOOST_REQUIRE(d6.is_extended());

	smf::mbus::dif d7(static_cast<std::uint8_t>(0x8D));
	BOOST_REQUIRE(d7.get_data_field_code() == smf::mbus::data_field_code::DFC_VAR);	//	variable length
	BOOST_REQUIRE(d7.get_function_field_code() == smf::mbus::function_field_code::INSTANT);
	BOOST_REQUIRE(!d7.is_storage());
	BOOST_REQUIRE(d7.is_extended());

}

BOOST_AUTO_TEST_CASE(reader)
{
	//
	//	Example from Annex P
	//	Table P.5 � SND-NR - Heat meter (wM-Bus) 
	//
	auto const inp = cyng::make_buffer(
		{ 0x0C	//	DIF (8 digit BCD)
		, 0x06	//	VIE (Energy kWh)
		, 0x27	//	Value LSB
		, 0x04	//	Value ( = 2850427 ==  002b7e7b)
		, 0x85	//	Value
		, 0x02	//	Value MSB

		//	-- record 2
		, 0x0C	//	DIE (8 digit BCD)
		, 0x13	//	VIE (Volume liter)
		, 0x76	//	Value LSB
		, 0x34	//	Value ( = 703476 == 000abbf4)
		, 0x70	//	Value
		, 0x00	//	Value MSB

		//	-- record 3
		, 0x4C	//	DIF (8 digit BCD, StorageNo 1)
		, 0x06	//	VIE (Energy kWh)
		, 0x19	//	Value LSB
		, 0x54	//	Value ( = 1445419 == 00160e2b)
		, 0x44	//	Value
		, 0x01	//	Value MSB

		//	-- record 4
		, 0x42	//	DIE (Data type G, StorageNo 1)
		, 0x6C	//	VIE (Date)
		, 0xFF	//	Value LSB
		, 0x0C	//	Value MSB ( = 31.12.2007)

		//	-- record 5
		, 0x0B	//	DIF (6 digit BCD)
		, 0x3B	//	VIF (Volume flow l/h)
		, 0x27	//	Value LSB	
		, 0x01	//	Value ( = 127 == 0000007f)
		, 0x00	//	Value MSB

		//	-- record 6
		, 0x0B	//	DIF (6 digit BCD)
		, 0x2A	//	VIF (Power 100 mW)
		, 0x97	//	Value LSB
		, 0x32	//	Value ( = 3297 == 00000ce1)
		, 0x00	//	Value MSB

		//	-- record 7
		, 0x0A	//	DIF (4 digit BCD)
		, 0x5A	//	VIF (Flow Temp. �C)
		, 0x43	//	Value LSB
		, 0x04	//	Value MSB ( = 443 == 01bb)

		//	-- record 8
		, 0x0A	//	DIF (4 digit BCD)
		, 0x5E	//	VIE (Return Temp. 100 m�C)
		, 0x51	//	Value LSB
		, 0x02	//	Value MSB ( = 251 == 00fb)

		//	-- record 9
		, 0x02	//	DIF (2 byte integer)
		, 0xFD	//	VIF (FD-Table)
		, 0x17	//	VIFE (error flag)
		, 0x00	//	Value LSB
		, 0x00	//	Value MSB ( = 0)
		//, 0x27	//	Fill Byte due to AES
		});

	smf::mbus::read(inp, 0);

}

BOOST_AUTO_TEST_CASE(bcd)
{
	auto const v1 = smf::mbus::bcd_to_n<std::uint32_t>(cyng::make_buffer(
		{ 0x27	//	Value LSB
		, 0x04	//	Value ( = 2850427 ==  002b7e7b)
		, 0x85	//	Value
		, 0x02	//	Value MSB
		}));
	BOOST_REQUIRE_EQUAL(v1, 2850427);
	auto const v2 = smf::mbus::bcd_to_n<std::uint32_t>(cyng::make_buffer(
		{ 0x76	//	Value LSB
		, 0x34	//	Value ( = 703476 == 000abbf4)
		, 0x70	//	Value
		, 0x00	//	Value MSB
		}));
	BOOST_REQUIRE_EQUAL(v2, 703476);
	auto const v3 = smf::mbus::bcd_to_n<std::uint32_t>(cyng::make_buffer(
		{ 0x19	//	Value LSB
		, 0x54	//	Value ( = 1445419 == 00160e2b)
		, 0x44	//	Value
		, 0x01	//	Value MSB
		}));
	BOOST_REQUIRE_EQUAL(v3, 1445419);
	auto const v5 = smf::mbus::bcd_to_n<std::uint16_t>(cyng::make_buffer(
		{ 0x27	//	Value LSB	
		, 0x01	//	Value ( = 127 == 0000007f)
		, 0x00	//	Value MSB
		}));
	BOOST_REQUIRE_EQUAL(v5, 127);
	auto const v6 = smf::mbus::bcd_to_n<std::uint16_t>(cyng::make_buffer(
		{ 0x97	//	Value LSB
		, 0x32	//	Value ( = 3297 == 00000ce1)
		, 0x00	//	Value MSB
		}));
	BOOST_REQUIRE_EQUAL(v6, 3297);
	auto const v7 = smf::mbus::bcd_to_n<std::uint16_t>(cyng::make_buffer(
		{ 0x43	//	Value LSB
		, 0x04	//	Value MSB ( = 443 == 01bb)
		}));
	BOOST_REQUIRE_EQUAL(v7, 443);
	auto const v8 = smf::mbus::bcd_to_n<std::uint16_t>(cyng::make_buffer(
		{ 0x51	//	Value LSB
		, 0x02	//	Value MSB ( = 251 == 00fb)
		}));
	BOOST_REQUIRE_EQUAL(v8, 251);

}

BOOST_AUTO_TEST_SUITE_END()