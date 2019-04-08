/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include "test-mbus-003.h"
#include <NODE_project_info.h>
#include <smf/mbus/parser.h>
#include <smf/mbus/header.h>
#include <smf/sml/srv_id_io.h>
#include <smf/mbus/defs.h>
#include <smf/mbus/aes.h>
#include <smf/mbus/variable_data_block.h>
#include <smf/sml/protocol/parser.h>

#include <cyng/io/serializer.h>
#include <cyng/io/io_buffer.h>
#include <cyng/tuple_cast.hpp>

#include <iostream>
#include <fstream>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace node 
{
	bool test_mbus_003()
	{
		std::array<char, 4> v{ 0x74, 0x31, 0x45, 0x04 };
		auto const idp = reinterpret_cast<std::uint32_t*>(v.data());

		std::stringstream ss;
		ss.fill('0');
		ss << std::setw(8) << std::setbase(16) << *idp;
		auto const s = ss.str();

		std::uint32_t id{ 0 };
		ss >> std::setbase(10) >> id;

		//	get data
		//auto p = boost::filesystem::path(NODE_SOURCE_DIRECTORY) / "test" / "unit-test" / "src" / "samples" / "mbus-003.bin";
		//auto p = boost::filesystem::path(NODE_SOURCE_DIRECTORY) / "test" / "unit-test" / "src" / "samples" / "mbus-004.bin";
		//auto p = boost::filesystem::path(NODE_SOURCE_DIRECTORY) / "test" / "unit-test" / "src" / "samples" / "mbus-005.bin";
		//auto p = boost::filesystem::path(NODE_SOURCE_DIRECTORY) / "test" / "unit-test" / "src" / "samples" / "mbus-snd-nr.bin";
		auto p = boost::filesystem::path(NODE_SOURCE_DIRECTORY) / "test" / "unit-test" / "src" / "samples" / "out.bin";
		std::ifstream ifs(p.string(), std::ios::binary | std::ios::app);
		if (ifs.is_open())
		{
			ifs >> std::noskipws;
			cyng::buffer_t data;
			data.insert(data.begin(), std::istream_iterator<char>(ifs), std::istream_iterator<char>());

			wmbus::parser p([](cyng::vector_t&& prg) {

				//	[op:ESBA,GWF,0,e,0009b5f8,72,13090016E61E3C07430020654C96AED902C8485C9E381D5AF2791A2818DF3763271C9EB2C29321E5EAC458C7,mbus.push.frame,op:INVOKE,op:REBA]
				//	[op:ESBA,01242396072000630E,HYD,63,e,0003105c,72,29436587E61EBF03B900200540C83A80A8E0668CAAB369804FBEFBA35725B34A55369C7877E42924BD812D6D,mbus.push.frame,op:INVOKE,op:REBA]
				//std::cout << cyng::io::to_str(prg) << std::endl;

				cyng::buffer_t data;
				auto const server_id = cyng::value_cast<>(prg.at(1), data);

				std::uint8_t const ci = cyng::value_cast<std::uint8_t>(prg.at(6), 0u);
				data = cyng::value_cast<>(prg.at(7), data);

				std::cout 
					<< sml::from_server_id(server_id)
					<< " - "
					<< mbus::get_medium_name(sml::get_medium_code(server_id))
					<< ": "
					<< std::hex
					<< +ci
					//<< ", initial vector: "
					//<< cyng::io::to_hex(int_vect, ' ')
					<< ' '
					<< cyng::io::to_hex(data, ' ')
					<< std::endl;

				if (ci == 0x72) {

					std::pair<header_long, bool> r = make_header_long(1, data);
					auto const server_id = r.first.get_srv_id();
					auto const manufacturer = sml::get_manufacturer_code(server_id);

					//auto const int_vect = mbus::build_initial_vector(r.first.get_iv());

					std::cout
						<< sml::decode(manufacturer)
						<< ": "
						<< mbus::get_medium_name(sml::get_medium_code(server_id))
						<< " - "
						<< sml::from_server_id(server_id)
						<< std::hex
						<< ", access #"
						<< +r.first.header().get_access_no()
						<< ", status: "
						<< +r.first.header().get_status()
						<< ", mode: "
						<< std::dec
						<< +r.first.header().get_mode()
						//<< ", initial vector: "
						//<< cyng::io::to_hex(int_vect, ' ')
						<< std::endl
						;

					if (sml::from_server_id(server_id) == "01-e61e-29436587-bf-03") {
						//	AES Key: 51 72 89 10 E6 6D 83 F8 51 72 89 10 E6 6D 83 F8
						BOOST_ASSERT(r.first.header().get_mode() == 5);

						//
						//	decode payload data
						//
						cyng::crypto::aes_128_key aes_key;
						aes_key.key_ = { 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8, 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8 };

						r.first.decode(aes_key);

						std::cout << cyng::io::to_hex(r.first.header().data(), ' ') << std::endl;

						auto counter = r.first.header().get_block_counter();
						node::vdb_reader reader;
						std::size_t offset{ 0 };

						while (counter-- != 0) {

							//
							//	decode block
							//
							offset = reader.decode(r.first.header().data(), offset);

							std::cout
								<< "meter "
								<< sml::from_server_id(server_id)
								<< ", value: "
								<< cyng::io::to_str(reader.get_value())
								<< ", scaler: "
								<< +reader.get_scaler()
								<< ", unit: "
								<< get_unit_name(reader.get_unit())
								<< std::endl;
						}

					}
				}
				else if (ci == 0x7F) {

					std::pair<header_short, bool> r = make_header_short(data);
					//auto const server_id = r.first.get_srv_id();
					auto const manufacturer = sml::get_manufacturer_code(server_id);

					std::cout
						<< sml::decode(manufacturer)
						<< ": "
						<< mbus::get_medium_name(sml::get_medium_code(server_id))
						<< " - "
						<< sml::from_server_id(server_id)
						<< std::hex
						<< ", access #"
						<< +r.first.get_access_no()
						<< ", status: "
						<< +r.first.get_status()
						<< ", mode: "
						<< std::dec
						<< +r.first.get_mode()
						<< ", counter: "
						<< +r.first.get_block_counter()
						<< std::endl
						;

					if (sml::from_server_id(server_id) == "01-a815-74314504-01-02") {

						//	
						//AES Key : 23 A8 4B 07 EB CB AF 94 88 95 DF 0E 91 33 52 0D 
						//IV	  : a8 15 74 31 45 04 01 02 e2 e2 e2 e2 e2 e2 e2 e2

						BOOST_ASSERT(r.first.get_mode() == 5);

						//
						//	decode payload data
						//
						cyng::crypto::aes_128_key aes_key;
						aes_key.key_ = { 0x23, 0xA8, 0x4B, 0x07, 0xEB, 0xCB, 0xAF, 0x94, 0x88, 0x95, 0xDF, 0x0E, 0x91, 0x33, 0x52, 0x0D };

						auto const iv = mbus::build_initial_vector(server_id, r.first.get_access_no());

						r.first.decode(aes_key, iv);
						std::cout << cyng::io::to_hex(r.first.data(), ' ') << std::endl;

						//
						//	ToDo: remove 0x2F from end of data buffer
						//

						//
						//	start SML parser
						//
						sml::parser sml_parser([](cyng::vector_t&& prg) {
							std::cout << cyng::io::to_str(prg) << std::endl;

							//	[op:ESBA,{E2,0,0,{0701,{null,01A815743145040102,null,null,{{0100010800FF,00020240,null,1e,-1,14521,null},{0100020800FF,00020240,null,1e,-1,552481,null},{0100010801FF,null,null,1e,-1,0,null},{0100020801FF,null,null,1e,-1,0,null},{0100010802FF,null,null,1e,-1,14521,null},{0100020802FF,null,null,1e,-1,552481,null},{0100100700FF,null,null,1b,-1,-2,null}},null,null}},5bb1},0,sml.msg,op:INVOKE,op:REBA]

						}, true, false);	//	verbose, no logging

						auto const data = r.first.data();
						sml_parser.read(data.begin(), data.end());
					}
				}


			});
			p.read(data.begin(), data.end());
			//p.reset();
		}

		return true;
	}

}
