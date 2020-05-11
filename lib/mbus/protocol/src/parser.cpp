/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/mbus/parser.h>
#include <smf/mbus/defs.h>
#include <cyng/vm/generator.h>
#include <cyng/util/slice.hpp>
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
			//, code_()
			, stream_buffer_()
			, input_(&stream_buffer_)
			, stream_state_(state::START)
			, parser_state_()
			, checksum_{ 0 }
			, length_{ 0 }
		{
			BOOST_ASSERT_MSG(cb_, "no callback specified");
			parser_state_ = ack();
		}

		parser::~parser()
		{
			if (cb_)	cb_ = nullptr;
		}


		void parser::reset()
		{
			stream_state_ = state::START;
			parser_state_ = ack();
		}


		char parser::put(char c)
		{
			switch (stream_state_)
			{
			case state::START:
                switch (static_cast<unsigned char>(c)) {
				case 0xE5:
					//	ack
					cb_(cyng::generate_invoke("mbus.ack", +c));
					break;
				case 0x10:
					checksum_ = 0;
					parser_state_ = short_frame();
					stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
					break;
				case 0x68:
					//	check frame type (control/long)
					checksum_ = 0;
					parser_state_ = test_frame();
					stream_state_ = state::FRAME;
					break;
				default:
					//	error
					BOOST_ASSERT_MSG(false, "m bus invalid start sequence");
					cb_(cyng::generate_invoke("log.msg.trace", "invalid start sequence: ", +c));
					break;
				}
				break;
			case state::FRAME:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				switch (stream_state_) {
				case state::FRAME_CTRL:
					parser_state_ = ctrl_frame();
					break;
				case state::FRAME_LONG:
					parser_state_ = long_frame();
					break;
				default:
					break;
				}
				checksum_ += c;
				break;
			default:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				checksum_ += c;
				if (state::START == stream_state_) {
					parser_state_ = ack();
				}
				break;
			}

			return c;
		}

		void parser::post_processing()
		{
			if (stream_state_ == state::START)
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
			return state::START;
		}

		parser::state parser::state_visitor::operator()(short_frame& frm) const
		{
			switch (frm.pos_) {
			case 0:	
				BOOST_ASSERT(c_ == 0x10);
				break;
			case 1:	//	C-field
				frm.c_ = c_;
				break;
			case 2:	//	A-field	
				frm.a_ = c_;
				break;
			case 3:	//	check sum
				frm.cs_ = c_;
				frm.ok_ = c_ == parser_.checksum_;
				//BOOST_ASSERT(c_ == frm.checksum_);
				break;
			case 4:
				BOOST_ASSERT(c_ == 0x16);
				parser_.cb_(cyng::generate_invoke("mbus.short.frame"
					, cyng::code::IDENT
					, frm.ok_	//	OK
					, frm.c_	//	C-field
					, frm.a_	//	A-field
					, frm.cs_));
				return state::START;
			default:
				return state::START;
			}

			++frm.pos_;
			return state::FRAME_SHORT;
		}
		parser::state parser::state_visitor::operator()(long_frame& frm) const
		{
			switch (frm.pos_) {
			case 0:
				BOOST_ASSERT(c_ == 0x68);
				break;
			case 1:	//	C-field
				frm.c_ = c_;
				break;
			case 2:	//	A-field	
				frm.a_ = c_;
				break;
			case 3:	//	CI-field
				frm.ci_ = c_;
				break;
			default:
				//	collect user data
				if (frm.pos_ <= parser_.length_) {
					//	user data 
					frm.data_.push_back(c_);
				}
				else if (parser_.length_ + 1 == frm.pos_) {
					//	
					//	check sum
					//	
					frm.cs_ = c_;
					frm.ok_ = c_ == parser_.checksum_;
					//BOOST_ASSERT(c_ == frm.checksum_);

					parser_.cb_(cyng::generate_invoke("mbus.long.frame"
						, cyng::code::IDENT
						, frm.ok_	//	OK
						, frm.c_	//	C-field
						, frm.a_	//	A-field
						, frm.ci_	//	CI-field
						, frm.cs_	//	check sum
						, frm.data_));	
				}
				else if (parser_.length_ + 2u == frm.pos_) {
					BOOST_ASSERT(c_ == 0x16);
					return state::START;
				}
			}

			frm.pos_++;
			return state::FRAME_LONG;
		}
		parser::state parser::state_visitor::operator()(ctrl_frame& frm) const
		{
			switch (frm.pos_) {
			case 0:
				BOOST_ASSERT(c_ == 0x68);
				break;
			case 1:	//	C-field
				frm.c_ = c_;
				break;
			case 2:	//	A-field	
				frm.a_ = c_;
				break;
			case 3:	//	CI-field
				frm.ci_ = c_;
				break;
			case 4:	//	check sum
				frm.cs_ = c_;
				frm.ok_ = c_ == parser_.checksum_;
				//BOOST_ASSERT(c_ == frm.checksum_);
				break;
			case 5:
				BOOST_ASSERT(c_ == 0x16);
				parser_.cb_(cyng::generate_invoke("mbus.ctrl.frame"
					, cyng::code::IDENT
					, frm.ok_	//	OK
					, frm.c_	//	C-field
					, frm.a_	//	A-field
					, frm.ci_	//	CI-field
					, frm.cs_));	//	check sum
				return state::START;
			default:
				return state::START;
			}

			frm.pos_++;
			return state::FRAME_CTRL;
		}
		parser::state parser::state_visitor::operator()(test_frame& frm) const
		{
			frm.lfield_.push_back(c_);
			if (2 == frm.lfield_.size()) {
				
				//	the length field (L field) is transmitted twice.
				BOOST_ASSERT(std::all_of(frm.lfield_.cbegin(), frm.lfield_.cend(), [&](char c) {
					return c == c_;
					}));

				if (std::all_of(frm.lfield_.cbegin(), frm.lfield_.cend(), [](char c) { 
					return c == 0x03; 
					})) {

					return 	parser::state::FRAME_CTRL;
				}

				parser_.length_ = c_;
				return parser::state::FRAME_LONG;
			}
			return parser::state::FRAME;
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
		using namespace node::mbus;

		//
		//	parser
		//
		parser::parser(parser_callback cb)
			: cb_(cb)
			, code_()
			, stream_buffer_()
			, input_(&stream_buffer_)
			, stream_state_(state::LENGTH)
			, parser_state_()
			, packet_size_(0)
			, version_(0)
			, medium_(0)
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
				std::cout 
					<< std::dec
					<< std::setfill('0')
					<< std::setw(8)
					<< m
					<< std::endl;
			}
#endif
		}

		void parser::reset()
		{
			stream_state_ = state::LENGTH;
			parser_state_ = error();
			packet_size_ = 0u;
			manufacturer_.clear();
			version_ = 0;
			medium_ = 0;
			dev_id_ = 0;
			server_id_.fill(0);
#ifdef _DEBUG
			std::cout << meter_set_.size() << " meter(s) read" << std::endl;
			for (auto m : meter_set_) {
				std::cout
					<< std::dec
					<< std::setfill('0')
					<< std::setw(8)
					<< m
					<< std::endl;
			}
			meter_set_.clear();
#endif

		}


		char parser::put(char c)
		{
			switch (stream_state_)
			{
			case state::LENGTH:
				//
				//	format A (S, T, R2 Mode): without CRC
				//	format B (C Mode): including CRC
				//
				//	T, C1A Mode: max 245 bytes
				//	C1B Mode: max 241 bytes
				//	R/S Mode: max 213 bytes
				//
				packet_size_ = static_cast<std::size_t>(c) & 0xFF;
				BOOST_ASSERT_MSG(packet_size_ > 9, "packet size to small");
				stream_state_ = state::CTRL_FIELD;
				break;
			case state::CTRL_FIELD:
				--packet_size_;
				std::tie(stream_state_, parser_state_) = ctrl_field(c);
				break;
			case state::MANUFACTURER:
				--packet_size_;
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				if (stream_state_ == state::DEV_ID) {
					parser_state_ = dev_id();
				}
				break;
			case state::DEV_ID:
				--packet_size_;
				//	address
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				break;
			case state::DEV_VERSION:
				//	Version (or Generation number) 
				--packet_size_;
				version_ = static_cast<std::uint8_t>(c);
				server_id_[7] = version_;
				stream_state_ = state::DEV_TYPE;
				break;
			case state::DEV_TYPE:
				//	Device type/Medium
				--packet_size_;
				medium_ = static_cast<std::uint8_t>(c);
				server_id_[8] = medium_;
				stream_state_ = state::FRAME_TYPE;
				break;
			case state::FRAME_TYPE:
				std::tie(stream_state_, parser_state_) = frame_type(c);
				break;
			case state::APL_ERROR:
			case state::HEADER_LONG:
			case state::HEADER_SHORT:
			case state::HEADER_NONE:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				break;
			default:
				cb_(cyng::generate_invoke("log.msg.warning", "invalid w-mbus parser state"));
				break;
			}

			return c;
		}

		std::pair<parser::state, parser::parser_state_t> parser::ctrl_field(char c)
		{
			switch (c) {
			case CTRL_FIELD_SND_NR:
				//	0x44 == Indicates message from primary station, function send / no reply(SND - NR)
				//stream_state_ = state::MANUFACTURER;
				//parser_state_ = manufacturer();	//	2 bytes
				return std::make_pair(state::MANUFACTURER, manufacturer());

			//case CTRL_FIELD_SND_IR:
				//	0x46 == Send manually initiated installation data (Send Installation Request - SND_IR)
				//	ED300L is sending 10 SND_IR packets after restart
				//break;

				//case CTRL_FIELD_ACC_NR: 
					//	0x47 == Contains no data – signals an empty transmission or provides the opportunity to access the bidirectional meter, between two application data transmissions
			//case CTRL_FIELD_ACC_DMD:
				//	0x48 == Access demand to master in order to request new important application data (alerts)
				//break;

			default:
				//stream_state_ = state::INVALID_STATE;
				cb_(cyng::generate_invoke("log.msg.error", "w-mbus: unknown control field: ", +c));
				break;
			}

			return std::make_pair(state::INVALID_STATE, error());
		}

		std::pair<parser::state, parser::parser_state_t> parser::frame_type(char c)
		{
			//	CI field of SND-NR frame
			//	0x72, 0x78 or 0x7A expected
			//	0x72: long data header
			//	0x7A: short data header
			//	0x78: no data header

			frame_type_ = static_cast<std::uint8_t>(c);
			cb_(cyng::generate_invoke("log.msg.trace", "frame type: ", frame_type_));

			switch (frame_type_) {
			case FIELD_CI_APL_ERROR:
				//	RSP_UD telegram
				return std::make_pair(state::APL_ERROR, frame_data(packet_size_));
			case FIELD_CI_HEADER_LONG:	//	0x72
				//	secondary address (8 bytes) + short header
				//
				return std::make_pair(state::HEADER_LONG, frame_data(packet_size_));
			case FIELD_CI_HEADER_SHORT:	//	0x7A
				//	byte1: counter (EN 13757-)
				//	byte2: status
				//	byte3/4: configuration (encryption mode and number of encrypted bytes)
				//
				return std::make_pair(state::HEADER_SHORT, frame_data(packet_size_));
			case FIELD_CI_HEADER_NO:	//	 0x78
				return std::make_pair(state::HEADER_NONE, frame_data(packet_size_));
			case FIELD_CI_EXT_DLL_I:	//	0x8C
				break;
			case FIELD_CI_NULL:	//	0xFF
				cb_(cyng::generate_invoke("log.msg.warning", "unsupported frame type FIELD_CI_NULL", frame_type_));
				break;
			case FIELD_CI_MANU_SPEC:	//	0xA0
				cb_(cyng::generate_invoke("log.msg.warning", "unsupported frame type FIELD_CI_MANU_SPEC", frame_type_));
				break;
			default:
				BOOST_ASSERT(c == 0x70 || c == 0x72 || c == 0x78 || c == 0x7A || c == 0x7F);
				break;
			}
			return std::make_pair(state::HEADER_NONE, frame_data(packet_size_));
		}

		void parser::post_processing()
		{
			//cyng::buffer_t const buffer(std::istreambuf_iterator<char>(input_), {});
			if (state::INVALID_STATE == stream_state_) {
				reset();
			}
		}

		parser::state_visitor::state_visitor(parser& p, char c)
			: parser_(p)
			, c_(c)
		{}

		parser::state parser::state_visitor::operator()(error&) const
		{
			return state::INVALID_STATE;
		}

		parser::state parser::state_visitor::operator()(manufacturer& v) const
		{
			v.data_[v.pos_++] = this->c_;
			this->parser_.server_id_[v.pos_] = this->c_;
			if (v.pos_ == v.data_.size()) {

				this->parser_.manufacturer_ = node::sml::decode(v.data_.at(0), v.data_.at(1));
				this->parser_.cb_(cyng::generate_invoke("log.msg.trace", "manufacturer: ", this->parser_.manufacturer_));
				return state::DEV_ID;
			}
			return state::MANUFACTURER;
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

				//
				//	read this value as a hex value
				//
				std::stringstream ss;
				ss.fill('0');
				ss << std::setw(8) << std::setbase(16) << *idp;

				//
				//	write this value as decimal value
				//
				ss >> std::setbase(10) >> this->parser_.dev_id_;
#ifdef _DEBUG
				this->parser_.cb_(cyng::generate_invoke("log.msg.trace", "device ID", this->parser_.dev_id_));

				if (this->parser_.meter_set_.find(this->parser_.dev_id_) == this->parser_.meter_set_.end()) {
					this->parser_.meter_set_.emplace(this->parser_.dev_id_);
					std::cout
						<< "new meter: "
						<< std::dec
						<< std::setfill('0')
						<< std::setw(8)
						<< this->parser_.dev_id_
						<< std::endl
						;
				}
#endif
				return state::DEV_VERSION;
			}
			return state::DEV_ID;
		}

		parser::frame_data::frame_data(std::size_t size)
			: size_(size - 1)
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

				parser_.cb_(cyng::generate_invoke("wmbus.push.frame"
					, cyng::buffer_t(this->parser_.server_id_.begin(), this->parser_.server_id_.end())
					, this->parser_.manufacturer_
					, this->parser_.version_
					, this->parser_.medium_
					, this->parser_.dev_id_
					, this->parser_.frame_type_
					, v.data_));

				return state::LENGTH;
			}
			return state::HEADER_NONE;
		}
	}
}	//	node

