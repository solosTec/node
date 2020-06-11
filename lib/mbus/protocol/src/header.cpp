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
	//
	//	header_base
	//	------------------------------------------------------------------+
	//

	header_base::header_base()
		: server_id_()
		, access_no_(0)
		, status_(0)
		, data_()
	{}

	header_base::header_base(srv_id_t const& server_id
		, std::uint8_t access_no
		, std::uint8_t status
		, cyng::buffer_t&& data)
	: server_id_(server_id)
		, access_no_(access_no)
		, status_(status)
		, data_(std::move(data))
	{}

	std::uint8_t header_base::get_access_no() const
	{
		return access_no_;
	}

	std::uint8_t header_base::get_status() const
	{
		return status_;
	}

	//
	//	header_long_wireless
	//	------------------------------------------------------------------+
	//

	bool header_long_wireless::verify_encryption() const
	{
		return test_aes(data_);
	}

	header_long_wireless::header_long_wireless()
		: header_base()
		, mode_(0)
		, block_counter_(0)
	{}

	header_long_wireless::header_long_wireless(srv_id_t const& id
		, std::uint8_t access_no
		, std::uint8_t status
		, std::uint8_t mode
		, std::uint8_t block_counter
		, cyng::buffer_t&& data)
	: header_base(id, access_no, status, std::move(data))
		, mode_(mode)
		, block_counter_(block_counter)
	{}

	header_long_wireless::header_long_wireless(srv_id_t const& id
		, wireless_prefix const& prefix)
	: header_base(id, prefix.get_access_no(), prefix.get_status(), prefix.get_data())
		, mode_(prefix.get_mode())
		, block_counter_(prefix.get_block_counter())
	{}

	wireless_prefix header_long_wireless::get_prefix() const
	{
		return wireless_prefix(access_no_
			, status_
			, mode_
			, block_counter_
			, data());
	}

	cyng::buffer_t header_long_wireless::get_srv_id() const
	{
		return cyng::buffer_t(server_id_.begin(), server_id_.end());
	}

	cyng::crypto::aes::iv_t header_long_wireless::get_iv() const
	{
		return mbus::build_initial_vector(get_srv_id(), get_access_no());
	}

	std::uint8_t header_long_wireless::get_mode() const
	{
		return mode_;
	}

	std::uint8_t header_long_wireless::get_block_counter() const
	{
		return block_counter_;
	}

	cyng::buffer_t header_long_wireless::data() const
	{
		return (verify_encryption())
			? cyng::buffer_t(data_.begin() + 2, data_.end())
			: data_
			;
	}

	cyng::buffer_t const& header_long_wireless::data_raw() const
	{
		return data_;
	}

	//cyng::buffer_t header_long_wireless::decode_mode_5(cyng::crypto::aes_128_key const& key) const
	//{
	//	return decode(data()
	//		, key
	//		, get_iv());
	//}


	//
	//	header_long_wired
	//	------------------------------------------------------------------+
	//

	header_long_wired::header_long_wired()
	: header_base()
		, signature_(0)
	{}

	header_long_wired::header_long_wired(srv_id_t const& id
		, std::uint8_t access_no
		, std::uint8_t status
		, std::uint16_t signature
		, cyng::buffer_t&& data)
	: header_base(id, access_no, status, std::move(data))
		, signature_(signature)
	{}

	cyng::buffer_t header_long_wired::get_srv_id() const
	{
		return cyng::buffer_t(server_id_.begin(), server_id_.end());
	}

	cyng::buffer_t header_long_wired::data() const
	{
		return data_;
	}

	std::uint16_t header_long_wired::get_signature() const
	{
		return signature_;
	}


	//
	//	wireless_prefix
	//	------------------------------------------------------------------+
	//

	wireless_prefix::wireless_prefix()
	: access_no_(0)
		, status_(0)
		, aes_mode_(0)
		, block_counter_(0)
		, data_()
	{}

	wireless_prefix::wireless_prefix(std::uint8_t access_no
		, std::uint8_t status
		, std::uint8_t aes_mode
		, std::uint8_t block_counter
		, cyng::buffer_t&& data)
	: access_no_(access_no)
		, status_(status)
		, aes_mode_(aes_mode)
		, block_counter_(block_counter)
		, data_(std::move(data))
	{}

	std::uint8_t wireless_prefix::get_access_no() const
	{
		return access_no_;
	}

	std::uint8_t wireless_prefix::get_status() const
	{
		return status_;
	}

	std::uint8_t wireless_prefix::get_mode() const
	{
		return aes_mode_;
	}

	std::uint8_t wireless_prefix::get_block_counter() const
	{
		return block_counter_;
	}

	std::uint8_t wireless_prefix::blocks() const
	{
		BOOST_ASSERT(block_counter_ < 17);
		return block_counter_ * 16u;
	}

	cyng::buffer_t wireless_prefix::get_data() const
	{
		return data_;
	}

	std::pair<cyng::buffer_t, bool> wireless_prefix::decode_mode_5(cyng::buffer_t const& server_id
		, cyng::crypto::aes_128_key const& key) const
	{
		return decode(data_
			, key
			, get_iv(server_id));
	}

	cyng::crypto::aes::iv_t wireless_prefix::get_iv(cyng::buffer_t const& server_id) const
	{
		return mbus::build_initial_vector(server_id, access_no_);
	}


	//
	//	free functions
	//	------------------------------------------------------------------+
	//

	std::pair<header_long_wired, bool> make_header_long_wired(cyng::buffer_t const& inp)
	{
		srv_id_t	server_id;
		server_id[0] = 1;

		if (inp.size() > 11) {

			auto pos = read_server_id(server_id, inp.cbegin(), inp.cend());

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

			return std::make_pair(header_long_wired(server_id, access_no, status, 0, cyng::buffer_t(pos, inp.end())), true);

		}

		return std::make_pair(header_long_wired{}, false);
	}

	std::pair<header_long_wireless, bool> make_header_long_wireless(cyng::buffer_t const& inp)
	{
		srv_id_t	server_id;
		server_id[0] = 1;

		if (inp.size() > 14) {

			//
			//	get server id
			//
			auto pos = read_server_id(server_id, inp.cbegin(), inp.cend());

			//
			//	get prefix
			//
			auto const prefix = make_wireless_prefix(pos, inp.cend());

			//
			//	build header
			//
			return std::make_pair(header_long_wireless(server_id
				, prefix.first), prefix.second);
		}

		return std::make_pair(header_long_wireless{}, false);
	}


	std::pair<cyng::buffer_t, bool> decode(cyng::buffer_t const& data
		, cyng::crypto::aes_128_key const& key
		, cyng::crypto::aes::iv_t const& iv)
	{
		//	Decode data with AES CBC (mode 5)

		//
		//	decrypt data
		//
		auto res = cyng::crypto::aes::decrypt(data, key, iv);

		//
		//	check success
		//
		if (test_aes(res)) {

			//
			//	remove trailing 0x2F
			//
			if (!res.empty()) {
				auto index = res.size();
				for (; index > 0 && res.at(index - 1) == 0x2F; --index);
				res.resize(index);
			}

			//
			//	substitute encrypted by decrypted data
			//
			return std::make_pair(res, true);
		}
		return std::make_pair(data, false);
	}

	bool test_aes(cyng::buffer_t const& buffer)
	{
		return (buffer.size() > 2) 
			&& (buffer.at(0) == 0x2F) 
			&& (buffer.at(1) == 0x2F);
	}

	cyng::buffer_t::const_iterator read_server_id(srv_id_t& srv_id, cyng::buffer_t::const_iterator pos, cyng::buffer_t::const_iterator end)
	{
		if (pos == end)	return pos;

		//
		//	Identification Number
		//	4 bytes serial ID
		//
		srv_id[3] = *pos++;
		if (pos == end)	return pos;
		srv_id[4] = *pos++;
		if (pos == end)	return pos;
		srv_id[5] = *pos++;
		if (pos == end)	return pos;
		srv_id[6] = *pos++;
		if (pos == end)	return pos;

		//
		//	Manufacturer Acronym / Code
		//
		srv_id[1] = *pos++;	//	4
		if (pos == end)	return pos;
		srv_id[2] = *pos++;	//	5
		if (pos == end)	return pos;

		//
		//	Version/Generation
		//
		srv_id[7] = *pos++;	//	6
		if (pos == end)	return pos;

		//
		//	Device type (Medium=HCA)
		//
		srv_id[8] = *pos++;	//	7

		return pos;
	}

	std::pair<wireless_prefix
		, bool> make_wireless_prefix(cyng::buffer_t const& data)
	{
		return make_wireless_prefix(std::begin(data), std::end(data));
	}

	std::pair<wireless_prefix
		, bool> make_wireless_prefix(cyng::buffer_t::const_iterator pos, cyng::buffer_t::const_iterator end)
	{
		if (pos == end)	return std::make_pair(wireless_prefix{}, false);

		//
		//	Access Number of Meter
		//
		std::uint8_t const access_no = static_cast<std::uint8_t>(*pos++);	//	0/8
		if (pos == end)	return std::make_pair(wireless_prefix(access_no, 0, 0, 0, cyng::buffer_t{}), false);

		//
		//	Meter state (Low power)
		//
		std::uint8_t const status = static_cast<std::uint8_t>(*pos++);	//	1/9
		if (pos == end)	return std::make_pair(wireless_prefix(access_no, status, 0, 0, cyng::buffer_t{}), false);

		//
		//	Config Field
		//	0 - no enryption
		//	5 - symmetric enryption (AES128 with CBC)
		//	7 - advanced symmetric enryption
		//	13 - assymetric enryption
		//
		//	currently only 0 (== no encryption) and 5 (== AES CBC) is supported
		//
		std::array<char, 2>   cfg;
		cfg[0] = *pos++;	//	2/10
		if (pos == end)	return std::make_pair(wireless_prefix(access_no, status, 0, 0, cyng::buffer_t{}), false);

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

		return std::make_pair(wireless_prefix(access_no, status, mode, block_counter, cyng::buffer_t(pos, end)), true);
	}


}	//	node

