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

	header_short::header_short(header_short const& other)
		: access_no_(other.get_access_no())
		, status_(other.get_status())
		, cfg_(other.cfg_)
		, data_(other.data_)
	{
	}
	
	header_short::header_short(header_short&& other)
	  : access_no_(other.access_no_)
	  , status_(other.status_)
	  , cfg_(other.cfg_)
	  , data_(std::move(other.data_))
	{
	}

	header_short::~header_short()
	{
	  data_.clear();
	}

	header_short& header_short::operator=(header_short const& other)
	{
		if (this != &other) {
			access_no_ = other.get_access_no();
			status_ = other.get_status();
			cfg_ = other.cfg_;
			data_.assign(other.data_.begin(), other.data_.end());
		}
		return *this;
	}
	
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
	  return (cfg_[1] & 0x1F);  // mask the first 5 bits to get the mode
	}

	std::uint8_t header_short::get_block_counter() const
	{
	  ucfg_fields cfg;
	  cfg.raw_[0] = cfg_[0];
	  cfg.raw_[1] = cfg_[1];

	  switch (get_mode()) {
	    case 5:	return cfg.cfg_field_5_.number_;
	    case 7: return cfg.cfg_field_7_.number_;
	    case 13: return cfg.cfg_field_D_.number_;

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

	std::size_t header_short::remove_aes_trailer()
	{
		std::size_t counter{ 0 };
		if (!data_.empty()) {
			while (data_.back() == 0x2F) {
				data_.pop_back();
				++counter;
			}
		}
		return counter;
	}

	std::pair<header_short, bool> make_header_short(cyng::buffer_t const& inp)
	{
		header_short hs;
		bool const b = reset_header_short(hs, inp);
		return std::make_pair(hs, b);
	}

	bool reset_header_short(header_short& hs, cyng::buffer_t const& inp)
	{
// 		std::cerr << "test-S"  << std::endl;
// 		std::size_t dbg{0};
// 		std::cerr << "(S" << dbg++ << ")" << std::endl;
		
		//
		//	reset
		//
		hs.access_no_ = 0;
		hs.status_ = 0;
		hs.cfg_[0] = 0;
		hs.cfg_[1] = 0;
		hs.data_.clear();
// 		std::cerr << "(S" << dbg++ << ") " << inp.size() << std::endl;
		
		if (inp.size() > 6) {

			auto pos = inp.cbegin();
// 			std::cerr << "(S" << dbg++ << ")" << std::endl;
			
			//
			//	Access Number of Meter
			//
			hs.access_no_ = static_cast<std::uint8_t>(*pos++);	//	0/8
// 			std::cerr << "(S" << dbg++ << ")" << std::endl;
			
			//
			//	Meter state (Low power)
			//
			hs.status_ = static_cast<std::uint8_t>(*pos++);	//	1/9
// 			std::cerr << "(S" << dbg++ << ")" << std::endl;
			
			//
			//	Config Field
			//	0 - no enryption
			//	5 - symmetric enryption (AES128 with CBC)
			//	7 - advanced symmetric enryption
			//	13 - assymetric enryption
			//
 			hs.cfg_[0] = *pos++;	//	2/10
 			hs.cfg_[1] = *pos++;	//	3/11
// 			std::cerr << "(S" << dbg++ << ")" << std::endl;
			
			//
			//	AES-Verify
			//	0x2F, 0x2F after decoding
			//

			//
			//	data field
			//
// 			std::cerr << "(S" << dbg++ << ") " << inp.size() << std::endl;
// 			std::cerr << "(S" << dbg++ << ") " << hs.data_.size() << std::endl;	//(7)
			hs.data_.assign(pos, inp.end());
// 			hs.data_ = inp;
// 			std::cerr << "(S" << dbg++ << ") " << hs.data_.size() << std::endl;
			return true;
		}
		return false;
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
	
	header_long::header_long(header_long&& other)
		: server_id_(other.server_id_)
		, hs_(std::move(other.hs_))
	{
	}

	header_long::~header_long()
	{}

	header_long& header_long::operator=(header_long const& other)
	{
		if (this != &other) {
			server_id_ = other.server_id_;
			hs_ = other.hs_;
		}
		return *this;
	}
	
	std::pair<header_long, bool> make_header_long(char type, cyng::buffer_t const& inp)
	{
		header_long hl;
		bool const b = reset_header_long(hl, type, inp);
		return std::make_pair(hl, b);
	}

	bool reset_header_long(header_long& hl, char type, cyng::buffer_t const& inp)
	{
// 		std::cerr << "test-L"  << std::endl;
// 		std::size_t dbg{0};
// 		std::cerr << "(L" << dbg++ << ")" << std::endl;
		//
		//	reset
		//
// 		std::cerr << "(L" << dbg++ << ")" << std::endl;
// 		std::cerr << "(L" << dbg++ << ")" << std::endl;
		hl.server_id_.fill(0u);
		hl.server_id_[0] = type;
		
		if (inp.size() > 14) {

// 			std::cerr << "(L" << dbg++ << ")" << std::endl;
			auto pos = inp.cbegin();

			//
			//	Identification Number
			//	4 bytes serial ID
			//
// 			std::cerr << "(L" << dbg++ << ")" << std::endl;
			hl.server_id_[3] = *pos++;
			hl.server_id_[4] = *pos++;
			hl.server_id_[5] = *pos++;
			hl.server_id_[6] = *pos++;

			//
			//	Manufacturer Acronym / Code
			//
// 			std::cerr << "(L" << dbg++ << ")" << std::endl;
			hl.server_id_[1] = *pos++;	//	4
			hl.server_id_[2] = *pos++;	//	5

			//
			//	Version/Generation
			//
// 			std::cerr << "(L" << dbg++ << ")" << std::endl;
			hl.server_id_[7] = *pos++;	//	6

			//
			//	Device type (Medium=HCA)
			//
// 			std::cerr << "(L" << dbg++ << ")" << std::endl;
			hl.server_id_[8] = *pos++;	//	7

			//
			//	build short header
			//
 			auto const sh = cyng::buffer_t(pos, inp.end());
// 			std::cerr << "(L" << dbg++ << ")" << std::endl;
			auto const b = reset_header_short(hl.hs_, sh);
// 			std::cerr << "(L" << dbg++ << ") " << (b ? "OK" : "FAILED") << std::endl;
			return b;
		}

		return false;
	}

	cyng::buffer_t header_long::get_srv_id() const
	{
		return cyng::buffer_t(server_id_.begin(), server_id_.end());
	}

	header_short const& header_long::header() const
	{
		return hs_;
	}

	header_short& header_long::header()
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

