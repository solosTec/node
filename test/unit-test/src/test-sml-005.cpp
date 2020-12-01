/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 
#include <test-sml-005.h>
#include <iostream>
#include <fstream>

#include <boost/test/unit_test.hpp>

#include <smf/sml/protocol/serializer.h>
#include "../../../lib/sml/protocol/src/writer.hpp"

#include <cyng/compatibility/file_system.hpp>
#include <cyng/factory.h>

namespace node 
{
	cyng::buffer_t write_read(cyng::filesystem::path p, std::string const& str)
	{
		//
		//	serialize string
		//
		std::ofstream ofs(p.c_str(), std::ios::binary);
		BOOST_CHECK(ofs.is_open());
		if (ofs.is_open())
		{
			sml::serialize(ofs, cyng::make_object(str));
		}
		ofs.close();

		//
		//	restore string
		//
		cyng::buffer_t buffer;
		std::ifstream ifs(p.c_str(), std::ios::binary);
		BOOST_CHECK(ifs.is_open());
		if (ifs.is_open())
		{
			ifs >> std::noskipws;
			buffer.insert(buffer.begin(), std::istream_iterator<std::uint8_t>(ifs), std::istream_iterator<uint8_t>());
		}
		return buffer;
	}

	bool test_sml_005()
	{
		{
			//	SML octet with a length of 48 bytes
			//	[8302]FEA5D6BC658D734673F5790051D61C2EBD55344A6285686DE3B645EE0104D65F5ABBA6CB68F1BFCB7FCF94F99CD4D845
			std::stringstream ss;
			sml::write_length_field(ss, 48, 0x00);

			char c[4] = { 0 };
			BOOST_CHECK_EQUAL(sizeof(c), 4);
			ss.read(c, sizeof(c));
			BOOST_CHECK_EQUAL(c[0], 0x83);
			BOOST_CHECK_EQUAL(c[1], 0x02);
		}
		{
			//	SML list with a length of 48 bytes
			std::stringstream ss;
			sml::write_length_field(ss, 48, 0x70);

			char c[4] = { 0 };
			BOOST_CHECK_EQUAL(sizeof(c), 4);
			ss.read(c, sizeof(c));
			BOOST_CHECK_EQUAL(c[0], 0xF3);
			BOOST_CHECK_EQUAL(c[1], 0x01);
		}

		//
		//	show length field for all possible values
		//
#ifdef __DEBUG
		for (std::uint32_t length = 0; length < std::numeric_limits<std::uint32_t>::max(); ++length) {
			std::stringstream ss;
			sml::write_length_field(ss, length, 0x70);
			//sml::write_length_field(ss, length, 0x00);

			std::cout << std::hex << length << '\t';
			while (true) {
				int c = ss.get();
				if (c == std::stringstream::traits_type::eof())	break;
				std::cout << (c & 0xFF) << ' ';

			}
			std::cout << std::endl;
		}
#endif

		//
		//	test serialization
		//

		//
		//	test a long string
		//	expected result is: 82 84 02  62 73 7A 3A 53 57 ...
		//

		std::string const str = "bsz:SW11132000-KF0007;authc:SW11132000-KF0007;ledio:SW11132000-KF0007;shdl:SW11132000-KF0007;smlc:SW11132000-KF0007;userif:SW11132000-KF0007;listc:SW11132000-KF0007;obislog:SW11132000-KF0007;landsl:SW11132000-KF0007;datacoll:SW11132000-KF0007;smlpush:SW11132000-KF0007;1107if:SW11132000-KF0007;ohdl_smldl:SW11132000-KF0007;meterif:SW11132000-KF0007;mbusif:SW11132000-KF0007;wmbif:SW11132000-KF0007;watcher:SW11132000-KF0007;update:SW11132000-KF0007;tswitch:SW11132000-KF0007;ohdl_muccfg:SW11132000-KF0007;lsmc:SW11132000-KF0007;wanipt:SW11132000-KF0007;gwa:SW11132000-KF0007;";
		auto p = cyng::filesystem::temp_directory_path() / cyng::filesystem::unique_path("test_sml_005-%%%%-%%%%-%%%%-%%%%.bin");

		auto r = write_read(p, str);
		BOOST_CHECK(!r.empty());
		BOOST_CHECK_EQUAL(r.size(), 0x242);
		BOOST_CHECK_EQUAL(r.size(), str.size() + 3);
		BOOST_CHECK_EQUAL((static_cast<unsigned>(r.at(0)) & 0xFF), 0x82);
		BOOST_CHECK_EQUAL((static_cast<unsigned>(r.at(1)) & 0xFF), 0x84);
		BOOST_CHECK_EQUAL((static_cast<unsigned>(r.at(2)) & 0xFF), 0x02);
		BOOST_CHECK_EQUAL((static_cast<unsigned>(r.at(3)) & 0xFF), 0x62);

		//
		//	test short string too
		//
		std::string const str_2 = "ABC";
		p = cyng::filesystem::temp_directory_path() / cyng::filesystem::unique_path("test_sml_005-%%%%-%%%%-%%%%-%%%%.bin");
		r = write_read(p, str_2);
		BOOST_CHECK(!r.empty());
		BOOST_CHECK_EQUAL((static_cast<unsigned>(r.at(0)) & 0xFF), 0x04);
		BOOST_CHECK_EQUAL(r.at(1), 'A');

		return true;
	}

}
