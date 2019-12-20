/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/mbus/header.h>
#include <smf/mbus/aes.h>

#include <cyng/factory.h>

#include <iostream>
#include <boost/assert.hpp>

namespace node 
{
	header_short::header_short()
	: access_no_(0)
		, status_(0)
		, mode_(0)
		, block_counter_(0)
		, data_()
	{}

	header_short::header_short(header_short const& other)
	: access_no_(other.get_access_no())
		, status_(other.get_status())
		, mode_(other.mode_)
		, block_counter_(other.block_counter_)
		, data_(other.data_)
	{
	}
	
	header_short::header_short(header_short&& other) noexcept
	: access_no_(other.access_no_)
		, status_(other.status_)
		, mode_(other.mode_)
		, block_counter_(other.block_counter_)
		, data_(std::move(other.data_))
	{
	}

	header_short::header_short(std::uint8_t access_no
		, std::uint8_t status
		, std::uint8_t mode
		, std::uint8_t block_counter
		, cyng::buffer_t&& data)
	: access_no_(access_no)
		, status_(status)
		, mode_(mode)
		, block_counter_(block_counter)
		, data_(std::move(data))
	{}

	header_short::~header_short()
	{}

	std::uint8_t header_short::get_access_no() const
	{
		return access_no_;
	}

	std::uint8_t header_short::get_status() const
	{
		return status_;
	}

	std::uint8_t header_short::get_mode() const
	{
		return mode_;
	}

	std::uint8_t header_short::get_block_counter() const
	{
		return block_counter_;
	}

	cyng::buffer_t header_short::data() const
	{
		return (verify_encryption())
			? cyng::buffer_t(data_.begin() + 2, data_.end())
			: data_
			;
	}

	cyng::buffer_t const& header_short::data_raw() const
	{
		return data_;
	}

	std::pair<header_short, bool> decode(header_short const& hs, cyng::crypto::aes_128_key const& key, cyng::crypto::aes::iv_t const& iv)
	{
		//
		//	test encryption mode
		//
		if (hs.get_mode() != 5)	return std::make_pair(hs, false);

		//
		//	decrypt data
		//
		auto data = cyng::crypto::aes::decrypt(hs.data_, key, iv);

		//
		//	check success
		//
		if (test_aes(data)) {

			//
			//	remove trailing 0x2F
			//
			if (!data.empty()) {
				auto index = data.size();
				for (; index > 0 && data.at(index - 1) == 0x2F; --index);
				data.resize(index);
			}

			//
			//	substitute encrypted by decrypted data
			//
			return std::make_pair(header_short(hs.get_access_no()
				, hs.get_status()
				, hs.get_mode()
				, hs.get_block_counter()
				, std::move(data)), true);
		}
		return std::make_pair(hs, false);
	}


	bool header_short::verify_encryption() const
	{
		return test_aes(data_);
	}

	std::pair<header_short, bool> make_header_short(cyng::buffer_t const& inp)
	{
	
		if (inp.size() > 6) {

			auto pos = inp.cbegin();
			
			//
			//	Access Number of Meter
			//
			std::uint8_t const access_no = static_cast<std::uint8_t>(*pos++);	//	0/8
			
			//
			//	Meter state (Low power)
			//
			std::uint8_t const status = static_cast<std::uint8_t>(*pos++);	//	1/9
			
			//
			//	Config Field
			//	0 - no enryption
			//	5 - symmetric enryption (AES128 with CBC)
			//	7 - advanced symmetric enryption
			//	13 - assymetric enryption
			//
			std::array<char, 2>   cfg;
 			cfg[0] = *pos++;	//	2/10
 			cfg[1] = *pos++;	//	3/11
			
			std::uint8_t mode = cfg[1] & 0x1F;
			std::uint8_t block_counter{ 0 };
			switch (mode) {
			case 0:
			case 5:
			case 7:
				block_counter = (cfg[0] & 0xF0) >> 4;
				break;
			case 13:
				block_counter = cfg[0];
				break;
			default:
				break;
			}

			//
			//	AES-Verify
			//	0x2F, 0x2F after decoding
			//

			//
			//	data field
			//
			return std::make_pair(header_short(access_no, status, mode, block_counter, cyng::buffer_t(pos, inp.end())), true);
		}
		return std::make_pair(header_short(), false);
	}

	header_long::header_long()
		: server_id_()
		, hs_()
	{}

	header_long::header_long(header_long const& other)
		: server_id_(other.server_id_)
		, hs_(other.hs_)
	{
	}
	
	header_long::header_long(header_long&& other) noexcept
		: server_id_(other.server_id_)
		, hs_(std::move(other.hs_))
	{
	}

	header_long::header_long(std::array<char, 9> const& id, header_short&& hs)
		: server_id_(id)
		, hs_(std::move(hs))
	{}

	header_long::~header_long()
	{}

	std::pair<header_long, bool> make_header_long(char type, cyng::buffer_t const& inp)
	{
		std::array<char, 9>	server_id;
		server_id[0] = type;
		
		if (inp.size() > 14) {

			auto pos = inp.cbegin();

			//
			//	Identification Number
			//	4 bytes serial ID
			//
			server_id[3] = *pos++;
			server_id[4] = *pos++;
			server_id[5] = *pos++;
			server_id[6] = *pos++;

			//
			//	Manufacturer Acronym / Code
			//
			server_id[1] = *pos++;	//	4
			server_id[2] = *pos++;	//	5

			//
			//	Version/Generation
			//
			server_id[7] = *pos++;	//	6

			//
			//	Device type (Medium=HCA)
			//
			server_id[8] = *pos++;	//	7

			//
			//	build short header
			//
			auto r =  make_header_short(cyng::buffer_t(pos, inp.end()));
			if (r.second) return std::make_pair(header_long(server_id, std::move(r.first)), true);
		}

		return std::make_pair(header_long(), false);
	}

	cyng::buffer_t header_long::get_srv_id() const
	{
		return cyng::buffer_t(server_id_.begin(), server_id_.end());
	}

	header_short const& header_long::header() const
	{
		return hs_;
	}

	cyng::crypto::aes::iv_t header_long::get_iv() const
	{
		return mbus::build_initial_vector(get_srv_id(), hs_.get_access_no());
	}

	std::pair<header_long, bool> decode(header_long const& hl, cyng::crypto::aes_128_key const& key)
	{
		auto r = decode(hl.header(), key, hl.get_iv());
		return std::make_pair(header_long(hl.server_id_, std::move(r.first)), r.second);
	}

	bool test_aes(cyng::buffer_t const& buffer)
	{
		return (buffer.size() > 2) 
			&& (buffer.at(0) == 0x2F) 
			&& (buffer.at(1) == 0x2F);
	}

}	//	node

