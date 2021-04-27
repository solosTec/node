#ifdef STAND_ALONE
#   define BOOST_TEST_MODULE unit_test
#endif

#include <boost/test/unit_test.hpp>

#include <smf/sml/parser.h>
#include <smf/sml/tokenizer.h>

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
		smf::sml::parser p([](std::string, std::uint8_t, std::uint8_t, smf::sml::msg_type, cyng::tuple_t, std::uint16_t) {
			});

		//	std::cout 
		//		<< smf::sml::get_name(type) 
		//		<< '['
		//		<< size
		//		<< '/'
		//		<< data.size() 
		//		<< ']'
		//		<< std::endl;

		//	});
		p.read(std::istream_iterator<char>(ifile), std::istream_iterator<char>());

	}

}

BOOST_AUTO_TEST_CASE(login)
{
	auto const inp = cyng::make_buffer({
		/*0x1b, 0x1b, 0x1b, 0x1b, 0x01, 0x01, 0x01, 0x01,*/ 0x76, 0x81, 0x04, 0x32, 0x31, 0x30, 0x34, 0x32, // ........v..21042
		0x37, 0x31, 0x30, 0x32, 0x38, 0x33, 0x32, 0x34, 0x36, 0x39, 0x30, 0x2d, 0x31, 0x62, 0x01, 0x62, // 71028324 690 - 1b.b
		0x00, 0x72, 0x63, 0xff, 0x01, 0x74, 0x08, 0x05, 0x00, 0x15, 0x3b, 0x01, 0xec, 0x46, 0x07, 0x81, // .rc..t.. ..; ..F..
		0x81, 0xc7, 0xc7, 0xfe, 0x03, 0x81, 0x01, 0x77, 0x72, 0x6f, 0x6e, 0x67, 0x20, 0x73, 0x65, 0x72, // .......w rong ser
		0x76, 0x65, 0x72, 0x20, 0x69, 0x64, 0x01, 0x63, 0x5a, 0x03, 0x00, 0x76, 0x81, 0x04, 0x32, 0x31, // ver id.c Z..v..21
		0x30, 0x34, 0x32, 0x37, 0x31, 0x30, 0x32, 0x38, 0x33, 0x32, 0x34, 0x36, 0x39, 0x30, 0x2d, 0x32, // 04271028 324690 - 2
		0x62, 0x02, 0x62, 0x00, 0x72, 0x63, 0x05, 0x01, 0x73, 0x08, 0x05, 0x00, 0x15, 0x3b, 0x01, 0xec, // b.b.rc..s....; ..
		0x46, 0x71, 0x07, 0x81, 0x00, 0x60, 0x05, 0x00, 0x00, 0x73, 0x07, 0x81, 0x00, 0x60, 0x05, 0x00, // Fq...`...s...`..
		0x00, 0x72, 0x62, 0x01, 0x54, 0x07, 0x02, 0x02, 0x01, 0x63, 0x06, 0x44, 0x00, 0x76, 0x81, 0x04, // .rb.T....c.D.v..
		0x32, 0x31, 0x30, 0x34, 0x32, 0x37, 0x31, 0x30, 0x32, 0x38, 0x33, 0x32, 0x34, 0x36, 0x39, 0x30, // 21042710 28324690
		0x2d, 0x33, 0x62, 0x03, 0x62, 0x00, 0x72, 0x63, 0x05, 0x01, 0x73, 0x08, 0x05, 0x00, 0x15, 0x3b, // - 3b.b.rc ..s....;
		0x01, 0xec, 0x46, 0x71, 0x07, 0x81, 0x81, 0xc7, 0x82, 0x01, 0xff, 0x73, 0x07, 0x81, 0x81, 0xc7, // ..Fq.... ...s....
		0x82, 0x01, 0xff, 0x01, 0x75, 0x73, 0x07, 0x81, 0x81, 0xc7, 0x82, 0x02, 0xff, 0x72, 0x62, 0x01, // ....us.. .....rb.
		0x07, 0x81, 0x81, 0xc7, 0x82, 0x53, 0xff, 0x01, 0x73, 0x07, 0x81, 0x81, 0xc7, 0x82, 0x03, 0xff, // .....S..s.......
		0x72, 0x62, 0x01, 0x09, 0x73, 0x6f, 0x6c, 0x6f, 0x73, 0x54, 0x65, 0x63, 0x01, 0x73, 0x07, 0x81, // rb..solo sTec.s..
		0x81, 0xc7, 0x82, 0x04, 0xff, 0x72, 0x62, 0x01, 0x08, 0x05, 0x00, 0x15, 0x5d, 0x8a, 0x6f, 0x2a, // .....rb. ....].o*
		0x01, 0x73, 0x07, 0x81, 0x81, 0xc7, 0x82, 0x06, 0xff, 0x01, 0x72, 0x73, 0x07, 0x81, 0x81, 0xc7, // .s...... ..rs....
		0x82, 0x07, 0x01, 0x01, 0x73, 0x73, 0x07, 0x81, 0x81, 0xc7, 0x82, 0x08, 0xff, 0x72, 0x62, 0x01, // ....ss.. .....rb.
		0x81, 0x01, 0x43, 0x55, 0x52, 0x52, 0x45, 0x4e, 0x54, 0x5f, 0x56, 0x45, 0x52, 0x53, 0x49, 0x4f, // ..CURREN T_VERSIO
		0x4e, 0x01, 0x73, 0x07, 0x81, 0x81, 0x00, 0x02, 0x00, 0x00, 0x72, 0x62, 0x01, 0x09, 0x30, 0x2e, // N.s..... ..rb..0.
		0x38, 0x2e, 0x33, 0x30, 0x36, 0x30, 0x01, 0x73, 0x07, 0x81, 0x81, 0xc7, 0x82, 0x0e, 0xff, 0x72, // 8.3060.s .......r
		0x62, 0x01, 0x42, 0x7f, 0x01, 0x73, 0x07, 0x81, 0x81, 0xc7, 0x82, 0x07, 0x02, 0x01, 0x73, 0x73, // b.B..s.. ......ss
		0x07, 0x81, 0x81, 0xc7, 0x82, 0x08, 0xff, 0x72, 0x62, 0x01, 0x07, 0x4b, 0x45, 0x52, 0x4e, 0x45, // .......r b..KERNE
		0x4c, 0x01, 0x73, 0x07, 0x81, 0x81, 0x00, 0x02, 0x00, 0x00, 0x72, 0x62, 0x01, 0x81, 0x0a, 0x77, // L.s..... ..rb...w
		0x69, 0x6e, 0x64, 0x6f, 0x77, 0x73, 0x20, 0x31, 0x30, 0x2e, 0x78, 0x20, 0x62, 0x75, 0x69, 0x6c, // indows 1 0.x buil
		0x64, 0x20, 0x31, 0x39, 0x30, 0x34, 0x31, 0x01, 0x73, 0x07, 0x81, 0x81, 0xc7, 0x82, 0x0e, 0xff, // d 19041. s.......
		0x72, 0x62, 0x01, 0x42, 0x7f, 0x01, 0x73, 0x07, 0x81, 0x81, 0xc7, 0x82, 0x09, 0xff, 0x01, 0x72, // rb.B..s. .......r
		0x73, 0x07, 0x81, 0x81, 0xc7, 0x82, 0x0a, 0x01, 0x72, 0x62, 0x01, 0x81, 0x02, 0x56, 0x53, 0x45, // s.......rb...VSE
		0x53, 0x2d, 0x31, 0x4b, 0x57, 0x2d, 0x32, 0x32, 0x31, 0x2d, 0x31, 0x46, 0x30, 0x01, 0x73, 0x07, // S - 1KW - 22 1 - 1F0.s.
		0x81, 0x81, 0xc7, 0x82, 0x0a, 0x02, 0x72, 0x62, 0x01, 0x09, 0x34, 0x34, 0x38, 0x39, 0x33, 0x39, // ......rb ..448939
		0x30, 0x39, 0x01, 0x63, 0x3a, 0xf5, 0x00, 0x76, 0x81, 0x04, 0x32, 0x31, 0x30, 0x34, 0x32, 0x37, // 09.c:..v ..210427
		0x31, 0x30, 0x32, 0x38, 0x33, 0x32, 0x34, 0x36, 0x39, 0x30, 0x2d, 0x34, 0x62, 0x00, 0x62, 0x00, // 10283246 90 - 4b.b.
		0x72, 0x63, 0x02, 0x01, 0x71, 0x01, 0x63, 0x1b, 0x6c, 0x00, /*0x00, 0x00, 0x1b, 0x1b, 0x1b, 0x1b,*/ // rc..q.c.l.......
		/*0x1a, 0x02, 0xfc, 0x30*/ });

	smf::sml::parser p([](std::string trx, std::uint8_t, std::uint8_t, smf::sml::msg_type type, cyng::tuple_t msg, std::uint16_t crc) {

		std::cout << "> " << smf::sml::get_name(type) << ": " << trx << ", " << msg << std::endl;

		});
	p.read(std::begin(inp), std::end(inp));

/*
### Message ###
76                                                SML_Message(Sequence):
  8104323130343237313032383332343639302D31        transactionId: 2104271028324690-1
  6201                                            groupNo: 1
  6200                                            abortOnError: 0
  72                                              messageBody(Choice):
    63FF01                                        messageBody: 65281 => SML_Attention_Res (0x0000FF01)
    74                                            SML_Attention_Res(Sequence):
      080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46
      078181C7C7FE03                              attentionNo: 81 81 C7 C7 FE 03
      810177726F6E6720736572766572206964          attentionMsg: wrong server id
      01                                          attentionDetails: not set
  635A03                                          crc16: c
  00                                              endOfSmlMsg: 00
### Message ###
76                                                SML_Message(Sequence):
  8104323130343237313032383332343639302D32        transactionId: 2104271028324690-2
  6202                                            groupNo: 2
  6200                                            abortOnError: 0
  72                                              messageBody(Choice):
    630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
    73                                            SML_GetProcParameter_Res(Sequence):
      080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46
      71                                          parameterTreePath(SequenceOf):
        07810060050000                            path_Entry: 81 00 60 05 00 00
      73                                          parameterTree(Sequence):
        07810060050000                            parameterName: 81 00 60 05 00 00
        72                                        parameterValue(Choice):
          6201                                    parameterValue: 1 => smlValue (0x01)
          54070202                                smlValue: 459266
        01                                        child_List: not set
  630644                                          crc16: 1604
  00                                              endOfSmlMsg: 00
### Message ###
76                                                SML_Message(Sequence):
  8104323130343237313032383332343639302D33        transactionId: 2104271028324690-3
  6203                                            groupNo: 3
  6200                                            abortOnError: 0
  72                                              messageBody(Choice):
    630501                                        messageBody: 1281 => SML_GetProcParameter_Res (0x00000501)
    73                                            SML_GetProcParameter_Res(Sequence):
      080500153B01EC46                            serverId: 05 00 15 3B 01 EC 46
      71                                          parameterTreePath(SequenceOf):
        078181C78201FF                            path_Entry: 81 81 C7 82 01 FF
      73                                          parameterTree(Sequence):
        078181C78201FF                            parameterName: 81 81 C7 82 01 FF
        01                                        parameterValue: not set
        75                                        child_List(SequenceOf):
          73                                      tree_Entry(Sequence):
            078181C78202FF                        parameterName: 81 81 C7 82 02 FF
            72                                    parameterValue(Choice):
              6201                                parameterValue: 1 => smlValue (0x01)
              078181C78253FF                      smlValue: ____S_
            01                                    child_List: not set
          73                                      tree_Entry(Sequence):
            078181C78203FF                        parameterName: 81 81 C7 82 03 FF
            72                                    parameterValue(Choice):
              6201                                parameterValue: 1 => smlValue (0x01)
              09736F6C6F73546563                  smlValue: solosTec
            01                                    child_List: not set
          73                                      tree_Entry(Sequence):
            078181C78204FF                        parameterName: 81 81 C7 82 04 FF
            72                                    parameterValue(Choice):
              6201                                parameterValue: 1 => smlValue (0x01)
              080500155D8A6F2A                    smlValue: 05 00 15 5D 8A 6F 2A
            01                                    child_List: not set
          73                                      tree_Entry(Sequence):
            078181C78206FF                        parameterName: 81 81 C7 82 06 FF
            01                                    parameterValue: not set
            72                                    child_List(SequenceOf):
              73                                  tree_Entry(Sequence):
                078181C7820701                    parameterName: 81 81 C7 82 07 01
                01                                parameterValue: not set
                73                                child_List(SequenceOf):
                  73                              tree_Entry(Sequence):
                    078181C78208FF                parameterName: 81 81 C7 82 08 FF
                    72                            parameterValue(Choice):
                      6201                        parameterValue: 1 => smlValue (0x01)
                      810143555252454E545F56455253494F4EsmlValue: CURRENT_VERSION
                    01                            child_List: not set
                  73                              tree_Entry(Sequence):
                    07818100020000                parameterName: 81 81 00 02 00 00
                    72                            parameterValue(Choice):
                      6201                        parameterValue: 1 => smlValue (0x01)
                      09302E382E33303630          smlValue: 0.8.3060
                    01                            child_List: not set
                  73                              tree_Entry(Sequence):
                    078181C7820EFF                parameterName: 81 81 C7 82 0E FF
                    72                            parameterValue(Choice):
                      6201                        parameterValue: 1 => smlValue (0x01)
                      427F                        smlValue: True
                    01                            child_List: not set
              73                                  tree_Entry(Sequence):
                078181C7820702                    parameterName: 81 81 C7 82 07 02
                01                                parameterValue: not set
                73                                child_List(SequenceOf):
                  73                              tree_Entry(Sequence):
                    078181C78208FF                parameterName: 81 81 C7 82 08 FF
                    72                            parameterValue(Choice):
                      6201                        parameterValue: 1 => smlValue (0x01)
                      074B45524E454C              smlValue: KERNEL
                    01                            child_List: not set
                  73                              tree_Entry(Sequence):
                    07818100020000                parameterName: 81 81 00 02 00 00
                    72                            parameterValue(Choice):
                      6201                        parameterValue: 1 => smlValue (0x01)
                      810A77696E646F77732031302E78206275696C64203139303431smlValue: windows 10.x build 19041
                    01                            child_List: not set
                  73                              tree_Entry(Sequence):
                    078181C7820EFF                parameterName: 81 81 C7 82 0E FF
                    72                            parameterValue(Choice):
                      6201                        parameterValue: 1 => smlValue (0x01)
                      427F                        smlValue: True
                    01                            child_List: not set
          73                                      tree_Entry(Sequence):
            078181C78209FF                        parameterName: 81 81 C7 82 09 FF
            01                                    parameterValue: not set
            72                                    child_List(SequenceOf):
              73                                  tree_Entry(Sequence):
                078181C7820A01                    parameterName: 81 81 C7 82 0A 01
                72                                parameterValue(Choice):
                  6201                            parameterValue: 1 => smlValue (0x01)
                  8102565345532D314B572D3232312D314630smlValue: VSES-1KW-221-1F0
                01                                child_List: not set
              73                                  tree_Entry(Sequence):
                078181C7820A02                    parameterName: 81 81 C7 82 0A 02
                72                                parameterValue(Choice):
                  6201                            parameterValue: 1 => smlValue (0x01)
                  093434383933393039              smlValue: 44893909
                01                                child_List: not set
  633AF5                                          crc16: 15093
  00                                              endOfSmlMsg: 00
### Message ###
76                                                SML_Message(Sequence):
  8104323130343237313032383332343639302D34        transactionId: 2104271028324690-4
  6200                                            groupNo: 0
  6200                                            abortOnError: 0
  72                                              messageBody(Choice):
    630201                                        messageBody: 513 => SML_PublicClose_Res (0x00000201)
    71                                            SML_PublicClose_Res(Sequence):
      01                                          globalSignature: not set
  631B6C                                          crc16: 7020
  00                                              endOfSmlMsg: 00
*/
}
BOOST_AUTO_TEST_SUITE_END()