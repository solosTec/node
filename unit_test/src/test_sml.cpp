#ifdef STAND_ALONE
#   define BOOST_TEST_MODULE unit_test
#endif

#include <boost/test/unit_test.hpp>

#include <smf/sml/parser.h>
#include <smf/sml/tokenizer.h>
#include <smf/sml/unpack.h>

#include <cyng/io/ostream.h>

#include <iostream>
#include <fstream>


BOOST_AUTO_TEST_SUITE(sml_suite)

BOOST_AUTO_TEST_CASE(tokenizer)
{
	std::ifstream ifile("..\\..\\unit_test\\assets\\smf--SML-202012T071425-power@solostec8bbb6264-bc4a4b92.sml", std::ios::binary);
	BOOST_CHECK(ifile.is_open());
	if (ifile.is_open())
	{
		//	dont skip whitespaces
		ifile >> std::noskipws;
		smf::sml::tokenizer p([](smf::sml::sml_type type, std::size_t size, cyng::buffer_t data) {

			std::cout
				<< smf::sml::get_name(type)
				<< '['
				<< size
				<< '/'
				<< data.size()
				<< ']'
				<< std::endl;

			});
		p.read(std::istream_iterator<char>(ifile), std::istream_iterator<char>());

	}

}

BOOST_AUTO_TEST_CASE(parser)
{
	std::ifstream ifile("..\\..\\unit_test\\assets\\smf--SML-202012T071425-power@solostec8bbb6264-bc4a4b92.sml", std::ios::binary);
	BOOST_CHECK(ifile.is_open());
	if (ifile.is_open())
	{
		//	dont skip whitespaces
		ifile >> std::noskipws;
		smf::sml::parser p([](std::string trx, std::uint8_t, std::uint8_t, smf::sml::msg_type type, cyng::tuple_t msg, std::uint16_t) {
			std::cout << "> " << smf::sml::get_name(type) << ": " << trx << ", " << msg << std::endl;
			});


		//	});
		p.read(std::istream_iterator<char>(ifile), std::istream_iterator<char>());

	}

}

BOOST_AUTO_TEST_CASE(login)
{
	auto const inp = cyng::make_buffer({
0x1b, 0x1b, 0x1b, 0x1b, 0x01, 0x01, 0x01, 0x01,/* trailer */ 0x76, 0x81, 0x05, 0x32, 0x31, 0x30, 0x34, 0x32, // ........ v..21042
0x37, 0x31, 0x36, 0x31, 0x37, 0x30, 0x34, 0x36, 0x38, 0x36, 0x35, 0x36, 0x2d, 0x31, 0x62, 0x00, // 71617046 8656-1b.
0x62, 0x00, 0x72, 0x63, 0x01, 0x00, 0x77, 0x01, 0x07, 0x00, 0x50, 0x56, 0xc0, 0x00, 0x08, 0x0f, // b.rc..w. ..PV....
0x32, 0x30, 0x32, 0x31, 0x30, 0x34, 0x32, 0x37, 0x31, 0x36, 0x31, 0x37, 0x30, 0x34, 0x08, 0x05, // 20210427 161704..
0x00, 0x15, 0x3b, 0x01, 0xec, 0x46, 0x09, 0x6f, 0x70, 0x65, 0x72, 0x61, 0x74, 0x6f, 0x72, 0x09, // ..;..F.o perator.
0x6f, 0x70, 0x65, 0x72, 0x61, 0x74, 0x6f, 0x72, 0x01, 0x63, 0x8d, 0x3b, 0x00, 0x76, 0x81, 0x05, // operator .c.;.v..
0x32, 0x31, 0x30, 0x34, 0x32, 0x37, 0x31, 0x36, 0x31, 0x37, 0x30, 0x34, 0x36, 0x38, 0x36, 0x35, // 21042716 17046865
0x36, 0x2d, 0x32, 0x62, 0x01, 0x62, 0x00, 0x72, 0x63, 0x05, 0x00, 0x75, 0x08, 0x05, 0x00, 0x15, // 6-2b.b.r c..u....
0x3b, 0x01, 0xec, 0x46, 0x09, 0x6f, 0x70, 0x65, 0x72, 0x61, 0x74, 0x6f, 0x72, 0x09, 0x6f, 0x70, // ;..F.ope rator.op
0x65, 0x72, 0x61, 0x74, 0x6f, 0x72, 0x71, 0x07, 0x81, 0x00, 0x60, 0x05, 0x00, 0x00, 0x01, 0x62, // eratorq. ..`....b
0x8d, 0x00, 0x76, 0x81, 0x05, 0x32, 0x31, 0x30, 0x34, 0x32, 0x37, 0x31, 0x36, 0x31, 0x37, 0x30, // ..v..210 42716170
0x34, 0x36, 0x38, 0x36, 0x35, 0x36, 0x2d, 0x33, 0x62, 0x02, 0x62, 0x00, 0x72, 0x63, 0x05, 0x00, // 468656-3 b.b.rc..
0x75, 0x08, 0x05, 0x00, 0x15, 0x3b, 0x01, 0xec, 0x46, 0x09, 0x6f, 0x70, 0x65, 0x72, 0x61, 0x74, // u....;.. F.operat
0x6f, 0x72, 0x09, 0x6f, 0x70, 0x65, 0x72, 0x61, 0x74, 0x6f, 0x72, 0x71, 0x07, 0x81, 0x81, 0xc7, // or.opera torq....
0x82, 0x01, 0xff, 0x01, 0x63, 0xb5, 0x0f, 0x00, 0x76, 0x81, 0x05, 0x32, 0x31, 0x30, 0x34, 0x32, // ....c... v..21042
0x37, 0x31, 0x36, 0x31, 0x37, 0x30, 0x34, 0x36, 0x38, 0x36, 0x35, 0x36, 0x2d, 0x34, 0x62, 0x00, // 71617046 8656-4b.
0x62, 0x00, 0x72, 0x63, 0x02, 0x00, 0x71, 0x01, 0x63, 0x37, 0x58, 0x00, /* post */ 0x1b, 0x1b, 0x1b, 0x1b, // b.rc..q. c7X.....
0x1a, 0x00, 0xf8, 0xad //                                      ....        
        });

	smf::sml::unpack p([](std::string trx, std::uint8_t, std::uint8_t, smf::sml::msg_type type, cyng::tuple_t msg, std::uint16_t crc) {

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
BOOST_AUTO_TEST_SUITE_END()