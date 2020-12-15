/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/mbus/parser.h>
#include <smf/mbus/defs.h>

#include <cyng/vm/generator.h>
//#include <cyng/util/slice.hpp>
#include <cyng/buffer_cast.h>

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
			, stream_buffer_()
			, input_(&stream_buffer_)
			, stream_state_(state::HEADER)
			, parser_state_()
			, header_pos_(0)
			, header_()
#ifdef _DEBUG
			, meter_set_()
#endif
		{
			BOOST_ASSERT_MSG(cb_, "no callback specified");
			parser_state_ = error();
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
			stream_state_ = state::HEADER;
			parser_state_ = error();
			header_pos_ = 0;
			header_.reset();
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
			case state::HEADER:
				header_.data_[header_pos_++] = c;
				if (header_.data_.size() == header_pos_) {

					//
					//	reset internal header position
					//
					header_pos_ = 0;

					//
					//	get next state
					//
					switch (header_.get_frame_type()) {
					case FIELD_CI_APL_ERROR:	
						stream_state_ = state::APL_ERROR;
						break;
					case FIELD_CI_HEADER_LONG:
						stream_state_ = state::HEADER_LONG;
						break;
					case FIELD_CI_HEADER_SHORT:
						stream_state_ = state::HEADER_SHORT;
						break;
					case FIELD_CI_HEADER_NO:
					case FIELD_CI_NULL:
					default:
						stream_state_ = state::HEADER_NONE;
						break;
					}
					parser_state_ = frame_data(header_.payload_size());

#ifdef _DEBUG
					if (meter_set_.find(header_.get_dev_id()) == meter_set_.end()) {
						meter_set_.emplace(header_.get_dev_id());
					}
#endif
				}
				break;
			case state::APL_ERROR:
			case state::HEADER_LONG:
			case state::HEADER_SHORT:
			case state::HEADER_NONE:
				stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
				break;
			default:
				//cb_(cyng::generate_invoke("log.msg.warning", "invalid w-mbus parser state"));
				break;
			}

			return c;
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

				parser_.cb_(this->parser_.header_, v.data_);

				return state::HEADER;
			}
			return state::HEADER_NONE;
		}

		header::header()
			: data_{0}
		{}

		std::uint8_t header::size() const
		{
			return data_.at(0);
		}

		std::uint8_t header::payload_size() const
		{
			return size() - 0x09;
		}

		std::uint8_t header::get_c_field() const
		{
			return data_.at(1);
		}

		cyng::buffer_t header::get_manufacturer_code() const
		{
			return { data_.at(2), data_.at(3) };
		}

		std::uint8_t header::get_version() const
		{
			return data_.at(8);
		}

		std::uint8_t header::get_medium() const
		{
			return data_.at(9);
		}

		std::uint32_t header::get_dev_id() const
		{
			std::uint32_t id{ 0 };

			//
			//	get the device ID as u32 value
			//
			//auto idp = reinterpret_cast<std::uint32_t const*>(dev_id_.data());
			auto p = reinterpret_cast<std::uint32_t const*>(&data_.at(4));

			//
			//	read this value as a hex value
			//
			std::stringstream ss;
			ss.fill('0');
			ss
				<< std::setw(8)
				<< std::setbase(16)
				<< *p;

			//
			//	write this value as decimal value
			//
			ss
				>> std::setbase(10)
				>> id
				;

			return id;
		}

		std::uint8_t header::get_frame_type() const
		{
			return data_.at(10);
		}

		void header::reset()
		{
			data_.fill(0);
		}

		cyng::buffer_t header::get_server_id() const
		{
			std::array<char, 9>	server_id;
			server_id[0] = 1;	//	wireless M-Bus

			//	[1-2] 2 bytes manufacturer ID
			server_id[1] = data_.at(2);
			server_id[2] = data_.at(3);

			//	[3-6] 4 bytes serial number (reversed)
			server_id[3] = data_.at(4);
			server_id[4] = data_.at(5);
			server_id[5] = data_.at(6);
			server_id[6] = data_.at(7);

			server_id[7] = data_.at(8);
			server_id[8] = data_.at(9);

			return cyng::buffer_t(server_id.begin(), server_id.end());
		}

		cyng::vector_t to_code(header const& h, cyng::buffer_t const& data)
		{
			return cyng::generate_invoke("wmbus.push.frame"
				, h.get_server_id()			//	9 bytes
				, h.get_manufacturer_code()	//	2 bytes
				, h.get_version()
				, h.get_medium()
				, h.get_dev_id()	//	u32
				, h.get_frame_type()
				, h.size()
				, data);
		}
	}

	cyng::buffer_t to_meter_id(std::uint32_t id)
	{
		//	Example: 0x3105c = > 96072000

		std::stringstream ss;
		ss.fill('0');
		ss
			<< std::setw(8)
			<< std::setbase(10)
			<< id;

		auto const s = ss.str();
		BOOST_ASSERT(s.size() == 8);

		ss
			>> std::setbase(16)
			>> id;

		return cyng::to_buffer<std::uint32_t>(id);
	}

}	//	node

