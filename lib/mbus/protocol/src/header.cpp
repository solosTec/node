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
		, cfg_()
		, data_()
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
		return cfg_.cfg_field.mode_;
	}

	std::uint8_t header_short::get_block_counter() const
	{
		switch (get_mode()) {
		case 5:	return cfg_.cfg_field_5_.number_;
		case 7: return cfg_.cfg_field_7_.number_;
		case 13: return cfg_.cfg_field_D_.number_;

		default:
			break;
		}
		return 0;
	}

	cyng::buffer_t header_short::data() const
	{
		return (verify_encryption())
			? cyng::buffer_t(data_.begin() + 2, data_.end())
			: data_
			;
	}

	bool header_short::decode(cyng::crypto::aes_128_key const& key, cyng::crypto::aes::iv_t const& iv)
	{
		//
		//	test encryption mode
		//
		if (get_mode() != 5)	return false;

		//
		//	decrypt data
		//
		auto dec = cyng::crypto::aes::decrypt(data_, key, iv);

		//
		//	check success
		//
		if (test_aes(dec))	{

			//
			//	substitute encrypted by decrypted data
			//
			data_ = dec;
			return true;
		}
		return false;
	}


	bool header_short::verify_encryption() const
	{
		return test_aes(data_);
	}

	std::pair<header_short, bool> make_header_short(cyng::buffer_t const& inp)
	{
		BOOST_ASSERT_MSG(inp.size() > 6, "header_long to short");
		header_short h;
		if (inp.size() > 6) {

			auto pos = inp.begin();

			//
			//	Access Number of Meter
			//
			h.access_no_ = *pos++;	//	0/8

			//
			//	Meter state (Low power)
			//
			h.status_ = *pos++;	//	1/9

			//
			//	Config Field
			//	0 - no enryption
			//	5 - symmetric enryption (AES128 with CBC)
			//	7 - advanced symmetric enryption
			//	13 - assymetric enryption
			//
			h.cfg_.raw_[0] = *pos++;	//	2/10
			h.cfg_.raw_[1] = *pos++;	//	3/11

			//
			//	AES-Verify
			//	0x2F, 0x2F after decoding
			//

			//
			//	data field
			//
			h.data_.insert(h.data_.end(), pos, inp.end());

			return std::make_pair(h, true);
		}
		return std::make_pair(h, false);
	}

	header_long::header_long()
		: server_id_()
		, hs_()
	{}

	std::pair<header_long, bool> make_header_long(char type, cyng::buffer_t const& inp)
	{
		BOOST_ASSERT_MSG(inp.size() > 14, "header_long to short");
		header_long h;

		h.server_id_[0] = type;

		if (inp.size() > 14) {

			auto pos = inp.begin();

			//
			//	Identification Number
			//	4 bytes serial ID
			//
			h.server_id_[3] = *pos++;
			h.server_id_[4] = *pos++;
			h.server_id_[5] = *pos++;
			h.server_id_[6] = *pos++;

			//
			//	Manufacturer Acronym / Code
			//
			h.server_id_[1] = *pos++;	//	4
			h.server_id_[2] = *pos++;	//	5

			//
			//	Version/Generation
			//
			h.server_id_[7] = *pos++;	//	6

			//
			//	Device type (Medium=HCA)
			//
			h.server_id_[8] = *pos++;	//	7

			//
			//	build short header
			//
			auto r = make_header_short(cyng::buffer_t(pos, inp.end()));
			h.hs_ = r.first;
			return std::make_pair(h, r.second);
		}

		return std::make_pair(h, false);
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

	bool header_long::decode(cyng::crypto::aes_128_key const& key)
	{
		return hs_.decode(key, get_iv());
	}

	bool test_aes(cyng::buffer_t const& buffer)
	{
		return (buffer.size() > 2) 
			&& (buffer.at(0) == 0x2F) 
			&& (buffer.at(1) == 0x2F);
	}

}	//	node

