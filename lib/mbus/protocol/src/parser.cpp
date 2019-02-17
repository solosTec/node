/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/mbus/parser.h>
#include <smf/mbus/defs.h>
#include <cyng/vm/generator.h>
#include <iostream>
#include <sstream>
#include <ios>
#include <iomanip>
#include <boost/numeric/conversion/cast.hpp>

namespace node 
{
	namespace mbus
	{

		//
		//	parser
		//
		parser::parser(parser_callback cb)
			: cb_(cb)
			, code_()
			, stream_buffer_()
			, input_(&stream_buffer_)
			, stream_state_(STATE_START)
			, parser_state_()
		{
			BOOST_ASSERT_MSG(cb_, "no callback specified");
			parser_state_ = ack();
		}

		parser::~parser()
		{
			if (cb_)	cb_ = nullptr;
			//if (f_read_uint8)	f_read_uint8 = nullptr;
		}


		void parser::reset()
		{
			stream_state_ = STATE_START;
			parser_state_ = ack();
		}


		char parser::put(char c)
		{
			switch (stream_state_)
			{
			case STATE_START:
                switch (static_cast<unsigned char>(c)) {
				case 0xE5:
					//	ack
					break;
				case 0x10:
					parser_state_ = short_frame();
					stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
					break;
				case 0x68:
					break;
				default:
					//	error
					BOOST_ASSERT_MSG(false, "m bus invalid start sequence");
					break;
				}
				break;
			case STATE_FRAME_SHORT:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				if (STATE_START == stream_state_) {
					cb_(std::move(code_));
				}
				break;
			//case STATE_SHORT_START:
			//	stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
			//	break;
			//case STATE_CONTROL_START:
			//	stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
			//	break;
			default:
				break;
			}

			return c;
		}

		void parser::post_processing()
		{
			if (stream_state_ == STATE_START)
			{
				//
				//	use 
				//	std::istreambuf_iterator<char> eos; 
				//	if {} doesn't work
				cyng::buffer_t buffer(std::istreambuf_iterator<char>(input_), {});
			}
		}

		parser::state_visitor::state_visitor(parser& p, char c)
			: parser_(p)
			, c_(c)
		{}

		parser::state parser::state_visitor::operator()(ack&) const
		{
			return STATE_START;
		}

		parser::state parser::state_visitor::operator()(short_frame& frm) const
		{
			switch (frm.pos_) {
			case 0:	
				BOOST_ASSERT(c_ == 0x10);
				break;
			case 1:	//	C-field
				parser_.input_.put(c_);
				frm.checksum_ += c_;
				break;
			case 2:	//	A-field	
				parser_.input_.put(c_);
				frm.checksum_ += c_;
				break;
			case 3:	//	check sum
				frm.ok_ = c_ == frm.checksum_;
				BOOST_ASSERT(c_ == frm.checksum_);
				break;
			case 4:
				BOOST_ASSERT(c_ == 0x16);
				parser_.code_ << cyng::generate_invoke("mbus.short.frame"
					, cyng::code::IDENT
					, frm.ok_	//	OK
					, parser_.get_read_uint8_f()	//	C-field
					, parser_.get_read_uint8_f()	//	A-field
					, parser_.get_read_uint8_f())	//	check sum
					<< cyng::unwind_vec();
				return STATE_START;
			default:
				return STATE_START;
			}

			++frm.pos_;
			return STATE_FRAME_SHORT;
		}
		parser::state parser::state_visitor::operator()(long_frame& frm) const
		{
			frm.pos_++;
			parser_.input_.put(c_);
			return STATE_START;
		}
		parser::state parser::state_visitor::operator()(ctrl_frame& frm) const
		{
			frm.pos_++;
			parser_.input_.put(c_);
			return STATE_START;
		}

		std::function<std::uint8_t()> parser::get_read_uint8_f()
		{
			return [&]()->std::uint8_t { return this->read_numeric<std::uint8_t>(); };
		}

	}

	//
	//	wireless m-bus
	//
	namespace wmbus
	{

		//
		//	parser
		//
		parser::parser(parser_callback cb)
			: cb_(cb)
			, code_()
			, stream_buffer_()
			, input_(&stream_buffer_)
			, stream_state_(STATE_LENGTH)
			, parser_state_()
			, packet_size_(0)
			, version_(0)
			, media_(0)
			, dev_id_(0)
			, server_id_()
#ifdef _DEBUG
			, meter_set_()
#endif
		{
			BOOST_ASSERT_MSG(cb_, "no callback specified");
			parser_state_ = error();
			server_id_[0] = 1;	//	wireless M-Bus
		}

		parser::~parser()
		{
			parser_callback pcb;
			cb_.swap(pcb);

#ifdef _DEBUG
			std::cout << meter_set_.size() << " meter(s) read" << std::endl;
			for (auto m : meter_set_) {
				std::cout << m << std::endl;
			}
#endif
		}

		void parser::reset()
		{
			stream_state_ = STATE_LENGTH;
			parser_state_ = error();
			packet_size_ = 0u;
			manufacturer_.clear();
			version_ = 0;
			media_ = 0;
			dev_id_ = 0;
			server_id_.fill(0);
#ifdef _DEBUG
			std::cout << meter_set_.size() << " meter(s) read" << std::endl;
			for (auto m : meter_set_) {
				std::cout << m << std::endl;
			}
			meter_set_.clear();
#endif

		}


		char parser::put(char c)
		{
			switch (stream_state_)
			{
			case STATE_LENGTH:
				packet_size_ = static_cast<std::size_t>(c) & 0xFF;
				BOOST_ASSERT_MSG(packet_size_ > 9, "packet size to small");
				stream_state_ = STATE_CTRL_FIELD;
				break;
			case STATE_CTRL_FIELD:
				BOOST_ASSERT_MSG(c == 0x44, "unknown control field");
				stream_state_ = STATE_MANUFACTURER;
				parser_state_ = manufacturer();
				break;
			case STATE_MANUFACTURER:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				if (stream_state_ == STATE_DEV_ID) {
					parser_state_ = dev_id();
				}
				break;
			case STATE_DEV_ID:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				if (stream_state_ == STATE_DEV_VERSION) {
					parser_state_ = dev_version();
				}
				break;
			case STATE_DEV_VERSION:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				break;
			case STATE_DEV_TYPE:
				media_ = boost::numeric_cast<std::uint8_t>(c);
				server_id_[7] = media_;
				stream_state_ = STATE_FRAME_TYPE;
				break;
			case STATE_FRAME_TYPE:
				//	0x72, 0x78 or 0x7A expected
				BOOST_ASSERT(c == 0x72 || c == 0x78 || c == 0x7A || c == 0x7F);
				frame_type_ = boost::numeric_cast<std::uint8_t>(c);
				stream_state_ = STATE_FRAME_DATA;
				parser_state_ = frame_data(packet_size_);
				break;
			case STATE_FRAME_DATA:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				break;
			default:
				break;
			}

			return c;
		}

		void parser::post_processing()
		{
			cyng::buffer_t buffer(std::istreambuf_iterator<char>(input_), {});
		}

		parser::state_visitor::state_visitor(parser& p, char c)
			: parser_(p)
			, c_(c)
		{}

		parser::state parser::state_visitor::operator()(error&) const
		{
			return STATE_ERROR;
		}

		parser::state parser::state_visitor::operator()(manufacturer& v) const
		{
			v.data_[v.pos_++] = this->c_;
			this->parser_.server_id_[v.pos_] = this->c_;
			if (v.pos_ == v.data_.size()) {

				this->parser_.manufacturer_ = node::sml::decode(v.data_.at(0), v.data_.at(1));
				//std::cout << "manufacturer: " << this->parser_.manufacturer_ << std::endl;
				return STATE_DEV_ID;
			}
			return STATE_MANUFACTURER;
		}

		parser::state parser::state_visitor::operator()(dev_version& v) const
		{
			v.u_.c_ = this->c_;
			//std::cout << "protocol type: " << +v.u_.internal_.type_ << ", protocol version: " << +v.u_.internal_.ver_ << std::endl;
			this->parser_.version_ = v.u_.internal_.ver_;
			this->parser_.server_id_[8] = v.u_.internal_.ver_;
			return STATE_DEV_TYPE;
		}

		parser::state parser::state_visitor::operator()(dev_id& v) const
		{
			v.data_[v.pos_++] = this->c_;
			this->parser_.server_id_[v.pos_ + 2] = this->c_;
			if (v.pos_ == v.data_.size()) {

				//
				//	get the device ID as u32 value
				//
				auto idp = reinterpret_cast<std::uint32_t*>(v.data_.data());
				//this->parser_.dev_id_ = (idp != nullptr) ? *idp : 0u;

				//
				//	read this value as a hex value
				//
				std::stringstream ss;
				ss.fill('0');
				ss << std::setw(8) << std::setbase(16) << *idp;
				//std::cout << "device id: " << ss.str() << std::endl;

				//
				//	write this value as decimal value
				//
				ss >> std::setbase(10) >> this->parser_.dev_id_;
#ifdef _DEBUG
				this->parser_.meter_set_.emplace(this->parser_.dev_id_);
#endif
				return STATE_DEV_VERSION;
			}
			return STATE_DEV_ID;
		}

		parser::frame_data::frame_data(std::size_t size)
			: size_(size - 10)
			, data_()
		{
			data_.reserve(size_);
		}

		void parser::frame_data::decode_main_vif(std::uint8_t vif)
		{
			if ((vif & 0x40) == 0) {
				decode_E0(vif);
			}
			else {
				decode_E1(vif);
			}
		}

		void parser::frame_data::decode_E0(std::uint8_t vif)
		{
			if ((vif & 0x020) == 0) {
				decode_E00(vif);
			}
			else {
				decode_01(vif);
			}
		}

		void parser::frame_data::decode_E00(std::uint8_t vif)
		{
			if ((vif & 0x10) == 0) {
				if ((vif & 0x08) == 0) {
					//	E000 0
					std::cout << "energy - multiplier: " << +((vif & 0x07) - 3) << ", Wh" << std::endl;
				}
				else {
					//	E000 1
					std::cout << "energy - multiplier: " << +(vif & 0x07) << ", Joule" << std::endl;
				}
			}
			else {

			}
		}

		void parser::frame_data::decode_01(std::uint8_t vif)
		{

		}

		void parser::frame_data::decode_E1(std::uint8_t vif)
		{
			if ((vif & 0x20) == 0) {
				decode_E10(vif);
			}
			else {
				decode_E11(vif);
			}
		}

		void parser::frame_data::decode_E10(std::uint8_t vif)
		{
			if ((vif & 0x010) == 0) {
				//	E100
				if ((vif & 0x08) == 0) {
					//	E100 0 - volume flow ext
					std::cout << "volume flow ext, multiplier: " << +((vif & 0x07) - 7) << ", cm3/min" << std::endl;
				}
				else {
					//	E100 1
					std::cout << "volume flow ext, multiplier: " << +((vif & 0x07) - 9) << ", cm3/sec" << std::endl;
				}
			}
		}

		void parser::frame_data::decode_E11(std::uint8_t vif)
		{
			if ((vif & 0x10) == 0) {
				decode_E110(vif);
			}
			else {
				decode_E111(vif);
			}
		}

		void parser::frame_data::decode_E110(std::uint8_t vif)
		{
		}

		void parser::frame_data::decode_E111(std::uint8_t vif)
		{
		}

		parser::state parser::state_visitor::operator()(frame_data& v) const
		{
			v.data_.push_back(this->c_);
			--v.size_;
			if (v.size_ == 0) {
				//std::cout << "frame with " << v.data_.size() << " bytes complete" << std::endl;

				parser_.cb_(cyng::generate_invoke("mbus.push.frame"
					, cyng::buffer_t(this->parser_.server_id_.begin(), this->parser_.server_id_.end())
					, this->parser_.manufacturer_
					, this->parser_.version_
					, this->parser_.media_
					, this->parser_.dev_id_
					, this->parser_.frame_type_
					, v.data_));

				return STATE_LENGTH;
			}
			return STATE_FRAME_DATA;
		}
	}
}	//	node

