/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/mbus/parser.h>
#include <cyng/vm/generator.h>
#include <iostream>
#include <ios>

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
			, f_read_uint8(std::bind(std::bind(&parser::read_numeric<std::uint8_t>, this)))
		{
			BOOST_ASSERT_MSG(cb_, "no callback specified");
			parser_state_ = ack();
		}

		parser::~parser()
		{
            parser_callback pcb;
            cb_.swap(pcb);
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
				switch (c) {
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
				//if (!buffer.empty())
				//{
				//	cb_(cyng::generate_invoke("ipt.req.transmit.data", cyng::code::IDENT, buffer));
				//}
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
					, parser_.f_read_uint8	//	C-field
					, parser_.f_read_uint8	//	A-field
					, parser_.f_read_uint8)	//	check sum
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

	}
}	//	node

