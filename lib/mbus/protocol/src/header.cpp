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
		std::cerr << "header_short::header_short(header_short const& other)" << std::endl;				
	}
	
	header_short::header_short(header_short&& other)
	  : access_no_(other.access_no_)
	  , status_(other.status_)
	  , cfg_(other.cfg_)
	  , data_(std::move(other.data_))
	{
	  std::cerr << "header_short::header_short(header_short&& other)" << std::endl;
	}

	header_short::~header_short()
	{
	  data_.clear();
	}

	header_short& header_short::operator=(header_short const& other)
	{
		std::cerr << "header_short& header_short::operator=(header_short const& other)...." << std::endl;
		try {
			std::cerr << "this : " << std::hex << reinterpret_cast<void*>(this) << std::endl;
			std::cerr << "other: " << std::hex << reinterpret_cast<void const*>(&other) << std::endl;
			if (this != &other) {
				std::cerr << "header_short& header_short::operator=(header_short const& other)... !=" << std::endl;
				std::cerr << "header_short& header_short::operator=(header_short const& other)... access no: " << +access_no_ << std::endl;
				access_no_ = other.get_access_no();
				std::cerr << "header_short& header_short::operator=(header_short const& other)... access no: " << +access_no_ << std::endl;

				std::cerr << "header_short& header_short::operator=(header_short const& other)... status: " << +status_ << std::endl;
				status_ = other.get_status();
				std::cerr << "header_short& header_short::operator=(header_short const& other)... status: " << +status_ << std::endl;
				
// 				cfg_ = other.cfg_;
				
				//  replace content
				std::cerr << "header_short& header_short::operator=(header_short const& other) - copy " << std::dec << other.data_.size() << " bytes" << std::endl;
				std::cerr << "this->data_ contains " << std::dec << this->data_.size() << " bytes" << std::endl;
//  			data_.assign(other.data_.begin(), other.data_.end());
				for (auto c : other.data_) {
					std::cerr << "this->data_ contains " << std::dec << this->data_.size() << " bytes + " << std::hex << +c << std::endl;
					this->data_.push_back(c);
				}
				std::cerr << "header_short& header_short::operator=(header_short const& other) " << this->data_.size() << " bytes copied " << std::endl;
			}
		}
		catch( std::exception const& ex) {
			std::cerr << ex.what() << std::endl;
		}
		std::cerr << "...header_short& header_short::operator=(header_short const& other)" << std::endl;		
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

	std::pair<header_short, bool> make_header_short(cyng::buffer_t inp)
	{
		BOOST_ASSERT_MSG(inp.size() > 6, "header_long to short");
		std::cerr << "make_header_short from " << std::dec << inp.size() << " bytes" << std::endl;
		header_short h;
		if (inp.size() > 6) {

			auto pos = inp.begin();

			//
			//	Access Number of Meter
			//
			h.access_no_ = static_cast<std::uint8_t>(*pos++);	//	0/8
			std::cerr << "Access Number of Meter" << std::endl;
			//
			//	Meter state (Low power)
			//
			h.status_ = static_cast<std::uint8_t>(*pos++);	//	1/9
			std::cerr << "Meter state" << std::endl;
			
			//
			//	Config Field
			//	0 - no enryption
			//	5 - symmetric enryption (AES128 with CBC)
			//	7 - advanced symmetric enryption
			//	13 - assymetric enryption
			//
			h.cfg_[0] = *pos++;	//	2/10
			h.cfg_[1] = *pos++;	//	3/11

			//
			//	AES-Verify
			//	0x2F, 0x2F after decoding
			//

			//
			//	data field
			//
			std::cerr << "data field... " << h.data_.size() << std::endl;
// 			h.data_.assign(pos, inp.end());
			while(pos < inp.end()) {
				std::cerr << "data field... " << std::dec << h.data_.size() << " + " << std::hex << +*pos << std::endl;
				h.data_.push_back(*pos);
				++pos;
			}
			std::cerr << "...data field with " << std::dec << h.data_.size() << " bytes" << std::endl;
			
			return std::make_pair(h, true);
		}
		return std::make_pair(h, false);
	}

	header_long::header_long()
		: server_id_()
		, hs_()
	{}

	header_long::header_long(header_long const& other)
		: server_id_(other.server_id_)
		, hs_(other.hs_)
	{
		std::cerr << "header_long::header_long(header_long const& other)" << std::endl;		
	}
	
	header_long::header_long(header_long&& other)
		: server_id_(other.server_id_)
		, hs_(std::move(other.hs_))
	{
		std::cerr << "header_long::header_long(header_long&& other)" << std::endl;
	}

	header_long::~header_long()
	{}

	header_long& header_long::operator=(header_long const& other)
	{
		std::cerr << "header_long& header_long::operator=(header_long const& other)..." << std::endl;		
		if (this != &other) {
			server_id_ = other.server_id_;
			hs_ = other.hs_;
		}
		std::cerr << "...header_long& header_long::operator=(header_long const& other)" << std::endl;		
		return *this;
	}
	
	std::pair<header_long, bool> make_header_long(char type, cyng::buffer_t inp)
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
			std::cerr << "build short header from " << std::dec << std::distance(pos, inp.end()) << " bytes" << std::endl;
// 			auto const sh = cyng::buffer_t(pos, inp.end());
			cyng::buffer_t sh;
			while(pos < inp.end()) {
				std::cerr << "buffer " << std::dec << sh.size() << " + " << std::hex << +*pos << std::endl;
				sh.push_back(*pos);
				++pos;
			}
			std::pair<header_short, bool> const r = make_header_short(sh);
			std::cerr << "short header " << (r.second ? "complete" : "INVALID") << std::endl;
			std::cerr << "short header has " << std::dec << h.hs_.data().size() << " bytes" << std::endl;
			h.hs_ = r.first;
			std::cerr << "long header complete" << std::endl;
			std::pair<header_long, bool> p(h, r.second);
// 			return std::make_pair(h, r.second);
			std::cerr << "return pair" << std::endl;
			return p;
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

