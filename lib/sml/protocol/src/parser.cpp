/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/sml/protocol/parser.h>
#include <smf/sml/crc16.h>

#include <cyng/vm/generator.h>
#include <cyng/io/hex_dump.hpp>
#include <cyng/io/serializer.h>

#include <iostream>
#include <ios>

namespace node 
{
	namespace sml
	{

		//
		//	parser
		//
		parser::parser(parser_callback cb, bool verbose, bool log)
			: cb_(cb)
			, verbose_(verbose)
			, log_(log)
			, pos_(0)
			, code_()
			, tl_()
			, offset_(0)
			, stream_state_(STATE_START)
			, parser_state_(sml_start())
			, crc_(crc_init())
			, crc_on_(true)
			, counter_(0)
			, stack_()
		{
			BOOST_ASSERT_MSG(cb_, "no callback specified");
		}

		parser::~parser()
		{
			if (cb_)	cb_ = nullptr;
		}


		void parser::reset()
		{
			stream_state_ = STATE_START;
			parser_state_ = sml_start();
			pos_ = 0;
			crc_on_ = true;
		}

		void parser::put(char c)
		{
			//	update crc
			if (crc_on_)
			{
				crc_ = crc_update(crc_, c);
			}

			auto prev_state = stream_state_;
			stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
			if (prev_state != stream_state_)
			{
				switch (stream_state_)
				{
				case STATE_START:
					parser_state_ = sml_start();
					break;
				case STATE_LENGTH:
					parser_state_ = sml_length();
					break;
				case STATE_STRING:
					parser_state_ = sml_string(tl_.length_ - offset_);
					break;
				case STATE_BOOLEAN:
					parser_state_ = sml_bool();
					break;
				case STATE_UINT8:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_uint8(tl_.length_ - offset_);
					break;
				case STATE_UINT16:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_uint16(tl_.length_ - offset_);
					break;
				case STATE_UINT32:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_uint32(tl_.length_ - offset_);
					break;
				case STATE_UINT64:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_uint64(tl_.length_ - offset_);
					break;
				case STATE_INT8:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_int8(tl_.length_ - offset_);
					break;
				case STATE_INT16:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_int16(tl_.length_ - offset_);
					break;
				case STATE_INT32:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_int32(tl_.length_ - offset_);
					break;
				case STATE_INT64:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_int64(tl_.length_ - offset_);
					break;
				case STATE_ESC:
					parser_state_ = sml_esc();
					break;
				case STATE_PROPERTY:
					parser_state_ = sml_prop();
					break;
				case STATE_START_STREAM:
					parser_state_ = sml_start_stream();
					break;
				case STATE_START_BLOCK:
					parser_state_ = sml_start_block();
					break;
				case STATE_TIMEOUT:
					parser_state_ = sml_timeout();
					break;
				case STATE_BLOCK_SIZE:
					parser_state_ = sml_block_size();
					break;
				case STATE_EOM:
					parser_state_ = sml_eom();
					break;

				default:
					{
						std::stringstream ss;
						ss
							<< stream_state_ 
							<< '@' 
							<< pos_
							;
						cb_(cyng::generate_invoke("log.msg.error", ss.str()));
					}
					break;
				}
			}
			pos_++;
		}

		void parser::post_processing()
		{
		}

		parser::state_visitor::state_visitor(parser& p, char c)
			: parser_(p)
			, c_(c)
		{}

		parser::state parser::state_visitor::operator()(sml_start& s) const
		{
			if (c_ == ESCAPE_SIGN)
			{
				return STATE_ESC;
			}
			else if (c_ == 0)
			{
				parser_.tl_.type_ = SML_EOM;
				parser_.tl_.length_ = 0;
				parser_.emit_eom();
			}
			else if (c_ == 1)
			{
				parser_.tl_.type_ = SML_OPTIONAL;
				parser_.tl_.length_ = 0;
				parser_.emit_nil();
			}
			else
			{
				parser_.offset_ = 1;
				parser_.tl_.length_ = (c_ & 0x0f);

				//	0x70: 0111 0000
				const char high = (c_ & 0x70) >> 4;
				switch (high)
				{
				case SML_STRING:
					parser_.tl_.type_ = SML_STRING;
					break;
				case SML_BOOLEAN:
					parser_.tl_.type_ = SML_BOOLEAN;
					break;
				case SML_INTEGER:
					parser_.tl_.type_ = SML_INTEGER;
					break;
				case SML_UNSIGNED:
					parser_.tl_.type_ = SML_UNSIGNED;
					break;
				case SML_LIST:
					parser_.tl_.type_ = SML_LIST;
					break;

				default:
					if (parser_.verbose_)
					{
						std::stringstream ss;
						ss
							<< "unknown type: "
							<< std::dec
							<< +high
							<< " @"
							<< std::hex
							<< parser_.pos_
							;
						parser_.cb_(cyng::generate_invoke("log.msg.error", ss.str()));
					}
					break;
				}

				if ((c_ & 0x80) == 0x80)
				{
					return STATE_LENGTH;
				}
				else
				{
					return parser_.next_state();
				}
			}

			return STATE_START;
		}

		parser::state parser::state_visitor::operator()(sml_length& s) const
		{
			//
			//	check MSB
			//
			//if (!(((c_ & 0x70) >> 4) == 0))
			//{
			//	//	error
			//	if (parser_.verbose_)
			//	{
			//		std::cerr 
			//			<< "***FATAL type "
			//			<< std::dec
			//			<< parser_.tl_.type_
			//			<< " during MSB check @"
			//			<< std::hex
			//			<< parser_.pos_
			//			<< std::endl;
			//	}

			//	return STATE_START;
			//}

			//
			//	calculate length.
			//	4 bits are used to encode the length info.
			//
			parser_.tl_.length_ *= 16;
			parser_.tl_.length_ |= (c_ & 0x0f);
			parser_.offset_++;

			//
			//	if MSB is on: more length fields are following
			//
			return ((c_ & 0x80) != 0x80)
				? parser_.next_state()
				: parser_.stream_state_
				;
		}
		parser::state parser::state_visitor::operator()(sml_string& s) const
		{
			if (s.push(c_))
			{
				//	emit() is the same as push() but with optional logging
				parser_.emit(std::move(s.octet_));
				return STATE_START;
			}
			return STATE_STRING;
		}
		parser::sml_string::sml_string(std::size_t size)
			: size_(size)
			, octet_()
		{
			octet_.reserve(size);
		}
		bool parser::sml_string::push(char c)
		{
			octet_.push_back(c);
			return octet_.size() == size_;
		}

		parser::state parser::state_visitor::operator()(sml_bool& s) const
		{
			if (parser_.verbose_ || parser_.log_)
			{
				std::stringstream ss;
				ss
					<< parser_.prefix()
					<< "BOOL "
					<< std::boolalpha
					<< (c_ != 0)
					;
				if (parser_.verbose_)
				{
					std::cerr
						<< ss.rdbuf()
						<< std::endl;
				}
				if (parser_.log_)
				{
					parser_.cb_(cyng::generate_invoke("sml.log", ss.str()));
				}
			}

			//	anything but zero is true
			parser_.push(cyng::make_object<bool>(c_ != 0));
			return STATE_START;
		}

		parser::state parser::state_visitor::operator()(sml_uint8& s) const
		{
			if (s.push(c_))
			{
				if (parser_.verbose_ || parser_.log_)
				{
					std::stringstream ss;
					ss
						<< parser_.prefix()
						<< "UINT8 "
						<< std::dec
						<< +s.u_.n_
					;
					if (parser_.verbose_)
					{
						std::cerr 
							<< ss.rdbuf() 
							<< std::endl;
					}
					if (parser_.log_)
					{
						parser_.cb_(cyng::generate_invoke("sml.log", ss.str()));
					}
				}
				parser_.push(cyng::make_object(s.u_.n_));
				return STATE_START;
			}
			return parser_.stream_state_;
		}
		parser::state parser::state_visitor::operator()(sml_uint16& s) const
		{
			if (s.push(c_))
			{
				if (parser_.verbose_ || parser_.log_)
				{
					std::stringstream ss;
					ss
						<< parser_.prefix()
						<< "UINT16 " 
						<< std::dec
						<< s.u_.n_
						;
					if (parser_.verbose_)
					{
						std::cerr
							<< ss.rdbuf()
							<< std::endl;
					}
					if (parser_.log_)
					{
						parser_.cb_(cyng::generate_invoke("sml.log", ss.str()));
					}
				}
				parser_.push(cyng::make_object(s.u_.n_));
				return STATE_START;
			}
			return parser_.stream_state_;
		}

		parser::state parser::state_visitor::operator()(sml_uint32& s) const
		{
			if (s.push(c_))
			{
				if (parser_.verbose_ || parser_.log_)
				{
					std::stringstream ss;
					ss
						<< parser_.prefix()
						<< "UINT32 "
						<< std::dec
						<< s.u_.n_
						;
					if (parser_.verbose_)
					{
						std::cerr
							<< ss.rdbuf()
							<< std::endl;
					}
					if (parser_.log_)
					{
						parser_.cb_(cyng::generate_invoke("sml.log", ss.str()));
					}
				}
				parser_.push(cyng::make_object(s.u_.n_));
				return STATE_START;
			}
			return parser_.stream_state_;
		}
		parser::state parser::state_visitor::operator()(sml_uint64& s) const
		{
			if (s.push(c_))
			{
				if (parser_.verbose_ || parser_.log_)
				{
					std::stringstream ss;
					ss
						<< parser_.prefix()
						<< "UINT64 "
						<< std::dec
						<< s.u_.n_
						;
					if (parser_.verbose_)
					{
						std::cerr
							<< ss.rdbuf()
							<< std::endl;
					}
					if (parser_.log_)
					{
						parser_.cb_(cyng::generate_invoke("sml.log", ss.str()));
					}
				}
				parser_.push(cyng::make_object(s.u_.n_));
				return STATE_START;
			}
			return parser_.stream_state_;
		}
		parser::state parser::state_visitor::operator()(sml_int8& s) const
		{
			if (s.push(c_))
			{
				if (parser_.verbose_ || parser_.log_)
				{
					std::stringstream ss;
					ss
						<< parser_.prefix()
						<< "INT8 "
						<< std::dec
						<< +s.u_.n_
						;
					if (parser_.verbose_)
					{
						std::cerr
							<< ss.rdbuf()
							<< std::endl;
					}
					if (parser_.log_)
					{
						parser_.cb_(cyng::generate_invoke("sml.log", ss.str()));
					}
				}
				parser_.push(cyng::make_object(s.u_.n_));
				return STATE_START;
			}
			return parser_.stream_state_;
		}
		parser::state parser::state_visitor::operator()(sml_int16& s) const
		{
			if (s.push(c_))
			{
				if (parser_.verbose_ || parser_.log_)
				{
					std::stringstream ss;
					ss
						<< parser_.prefix()
						<< "INT16 "
						<< s.u_.n_
						;
					if (parser_.verbose_)
					{
						std::cerr
							<< ss.rdbuf()
							<< std::endl;
					}
					if (parser_.log_)
					{
						parser_.cb_(cyng::generate_invoke("sml.log", ss.str()));
					}
				}
				parser_.push(cyng::make_object(s.u_.n_));
				return STATE_START;
			}
			return parser_.stream_state_;
		}
		parser::state parser::state_visitor::operator()(sml_int32& s) const
		{
			if (s.push(c_))
			{
				if (parser_.verbose_ || parser_.log_)
				{
					std::stringstream ss;
					ss
						<< parser_.prefix()
						<< "INT32 "
						<< s.u_.n_
						;
					if (parser_.verbose_)
					{
						std::cerr
							<< ss.rdbuf()
							<< std::endl;
					}
					if (parser_.log_)
					{
						parser_.cb_(cyng::generate_invoke("sml.log", ss.str()));
					}
				}
				parser_.push(cyng::make_object(s.u_.n_));
				return STATE_START;
			}
			return parser_.stream_state_;
		}
		parser::state parser::state_visitor::operator()(sml_int64& s) const
		{
			if (s.push(c_))
			{
				if (parser_.verbose_ || parser_.log_)
				{
					std::stringstream ss;
					ss
						<< parser_.prefix()
						<< "INT64 "
						<< s.u_.n_
						;
					if (parser_.verbose_)
					{
						std::cerr
							<< ss.rdbuf()
							<< std::endl;
					}
					if (parser_.log_)
					{
						parser_.cb_(cyng::generate_invoke("sml.log", ss.str()));
					}
				}
				parser_.push(cyng::make_object(s.u_.n_));
				return STATE_START;
			}
			return parser_.stream_state_;
		}

		parser::state parser::state_visitor::operator()(sml_esc& s) const
		{
			if (c_ == ESCAPE_SIGN)
			{
				s.counter_++;
				if (s.counter_ == 4)
				{
					return STATE_PROPERTY;
				}
			}
			else
			{
				if (parser_.verbose_)
				{
					std::stringstream ss;
					ss
						<< "invalid TL field @"
						<< std::hex
						<< parser_.pos_
						;
					parser_.cb_(cyng::generate_invoke("log.msg.fatal", ss.str()));
				}

				//
				//	invalid TL field
				//
				return STATE_START;

			}
			return parser_.stream_state_;
		}
		parser::sml_esc::sml_esc()
			: counter_(1)
		{}

		parser::state parser::state_visitor::operator()(sml_prop& s) const
		{
			switch (c_)
			{
			case 0x01:	return STATE_START_STREAM;
			case 0x02:	return STATE_START_BLOCK;
			case 0x03:	return STATE_TIMEOUT;
			case 0x04:	return STATE_BLOCK_SIZE;
			case 0x1a:	return STATE_EOM;
			default:
			{
				std::stringstream ss;
				ss
					<< "escape property "
					<< +c_
					<< " not supported"
					;
				parser_.cb_(cyng::generate_invoke("log.msg.error", ss.str()));
			}
				break;
			}

			return parser_.stream_state_;
		}

		parser::state parser::state_visitor::operator()(sml_start_stream& s) const
		{
			s.counter_++;
			if (s.counter_ == 4)
			{
				if (parser_.verbose_)
				{
					std::cerr
						<< "start data stream @"
						<< std::hex
						<< parser_.pos_
						<< std::endl;
				}

				//
				//	start new message
				//
				return STATE_START;
			}
			return parser_.stream_state_;
		}
		parser::sml_start_stream::sml_start_stream()
			: counter_(1)
		{}

		parser::state parser::state_visitor::operator()(sml_start_block& s) const
		{
			if (s.push(c_))
			{
				if (parser_.verbose_)
				{
					std::cerr
						<< "start block transfer @"
						<< std::hex
						<< parser_.pos_
						<< std::endl;
				}
				return  STATE_START;
			}
			return parser_.stream_state_;
		}
		parser::sml_start_block::sml_start_block()
			: prop_()
		{
			prop_.push_back(2);
		}
		bool parser::sml_start_block::push(char c)
		{
			prop_.push_back(c);
			return prop_.size() == 4;
		}

		parser::state parser::state_visitor::operator()(sml_timeout& s) const
		{
			if (s.push(c_))
			{
				if (parser_.verbose_)
				{
					std::cerr
						<< "timeout @"
						<< std::hex
						<< parser_.pos_
						<< std::endl;
				}
				return  STATE_START;
			}
			return parser_.stream_state_;
		}
		parser::sml_timeout::sml_timeout()
			: prop_()
		{
			prop_.push_back(3);
		}
		bool parser::sml_timeout::push(char c)
		{
			prop_.push_back(c);
			return prop_.size() == 4;
		}

		parser::state parser::state_visitor::operator()(sml_block_size& s) const
		{
			if (s.push(c_))
			{
				if (parser_.verbose_)
				{
					std::cerr
						<< "blocksize @"
						<< parser_.pos_
						<< std::endl;
				}
				return  STATE_START;
			}
			return parser_.stream_state_;
		}
		parser::sml_block_size::sml_block_size()
			: prop_()
		{
			prop_.push_back(3);
		}
		bool parser::sml_block_size::push(char c)
		{
			prop_.push_back(c);
			return prop_.size() == 4;
		}

		parser::state parser::state_visitor::operator()(sml_eom& s) const
		{
			if (s.counter_ == 1)
			{
				parser_.crc_on_ = false;
				//	finalize CRC calculation
				parser_.crc_ = crc_finalize(parser_.crc_);
			}
			if (s.push(c_))
			{
				if (parser_.verbose_)
				{
					std::stringstream ss;
					ss
						<< "end of message "
						<< std::dec
						<< +s.pads_
						<< " pad(s), CRC16: "
						<< std::hex
						<< s.crc_
						<< "/"
						<< parser_.crc_
						<< " @"
						<< parser_.pos_
						;

					parser_.cb_(cyng::generate_invoke("log.msg.debug", ss.str()));
				}

				parser_.crc_on_ = true;
				parser_.finalize(s.crc_, s.pads_);
				return STATE_START;
			}
			return parser_.stream_state_;
		}
		parser::sml_eom::sml_eom()
			: counter_(1)
			, crc_(0)
			, pads_(0)
		{}
		bool parser::sml_eom::push(char c)
		{
			switch (counter_)
			{
			case 1:
				pads_ = c;
				break;
			case 2:
				crc_ = c;
				crc_ <<= 8;
				break;
			case 3:
				crc_ |= (c & 0xFF);
				break;
			default:
				break;
			}
			counter_++;
			return counter_ == 4;
		}

		parser::state parser::next_state()
		{
			switch (tl_.type_)
			{
			case SML_STRING:	return STATE_STRING;
			case SML_BOOLEAN:	return STATE_BOOLEAN;
			case SML_INTEGER:
				switch (tl_.length_)
				{
				case 2:	
					return STATE_INT8;
				case 3:	
					return STATE_INT16;
				case 4: case 5: 
					return STATE_INT32;
				case 6: case 7: case 8: case 9:
					return STATE_INT64;
				default:
					BOOST_ASSERT_MSG(false, "invalid integer length");
					break;
				}
				break;
			case SML_UNSIGNED:	
				switch (tl_.length_)
				{
				case 2:
					return STATE_UINT8;
				case 3:
					return STATE_UINT16;
				case 4: case 5:
					return STATE_UINT32;
				case 6: case 7: case 8: case 9:
					return STATE_UINT64;
				default:
					BOOST_ASSERT_MSG(false, "invalid unsigned length");
					break;
				}
				break;
			case SML_LIST:
				//
				//	prepare new list
				//
				if (verbose_ || log_) {
					std::stringstream ss;
					ss
						<< "open list with "
						<< std::dec
						<< stack_.size()
						<< ':'
						<< tl_.length_
						<< " entries @"
						<< std::hex
						<< pos_
// 						<< std::endl
						;
					
					if (verbose_)
					{
						std::cerr
							<< ss.str()
							<< std::endl;
					}
					if (log_)
					{
						cb_(cyng::generate_invoke("sml.log", ss.str()));
					}
				}
				if (tl_.length_ == 0) {
					//
					//	empty list as empty tuple
					//
					push(cyng::tuple_factory());
				}
				else {
					stack_.push(list(tl_.length_));
				}
				return STATE_START;
			default:
			{
				std::stringstream ss;
				ss
					<< "unknown data type "
					<< tl_.type_
					<< " @"
					<< std::hex
					<< pos_
					;
				cb_(cyng::generate_invoke("log.msg.fatal", ss.str()));
			}
				BOOST_ASSERT_MSG(false, "unknown data type");
				break;
			}
			return stream_state_;
		}

		void parser::emit_eom()
		{
			if (stack_.size() > 1)
			{
				std::stringstream ss;
				ss
					<< "EOM with stack size: "
					<< stack_.size()
					<< " @"
					<< std::hex
					<< pos_
					;
				cb_(cyng::generate_invoke("log.msg.error", ss.str()));
			}

			if (verbose_)
			{
				std::cerr << prefix()
					<< "***EOM***"
					<< " (depth: "
					<< stack_.size()
					<< ")"
					<< std::endl
					;
			}

			if (!stack_.empty())
			{
				//
				//	generate SML message
				//
				cyng::vector_t prg{ cyng::generate_invoke("sml.msg", stack_.top().values_, counter_) };

				//
				//	remove accumulated  values from stack
				//
				stack_.pop();

				if (verbose_)
				{
					std::cerr 
						<< "*** SML message complete"
						<< std::endl;

				}

				//
				//	message complete
				//
				cb_(std::move(prg));
				counter_++;
			}
		}


		void parser::emit_nil()
		{
			if (verbose_ || log_) {
				std::stringstream ss;
				ss 
					<< prefix() 
					<< "[OPTIONAL]"
					;
				
				if (verbose_)
				{
					std::cerr 
						<< ss.str()
						<< std::endl;
					;
				}
				if (log_) 
				{
					cb_(cyng::generate_invoke("sml.log", ss.str()));
				}
			}
			push(cyng::make_object());		
		}

		void parser::emit(cyng::buffer_t&& octet)
		{
			if (verbose_)
			{
				cyng::io::hex_dump hd(8);
				std::stringstream ss;
				hd(ss, octet.data(), octet.data() + octet.size());
				std::cerr 
					<< prefix()
					<< "STRING: "
					<< tl_.length_
					<< " bytes\t"
					<< ss.str()
				;
			}
			if (verbose_ || log_)
			{
				std::stringstream ss;
				ss
					<< prefix()
					<< "STRING: "
					<< tl_.length_
					<< " bytes\t"
					;

				cyng::io::hex_dump hd(8);
				hd(ss, octet.data(), octet.data() + octet.size());

				if (verbose_)
				{
					std::cerr
						<< ss.rdbuf()
						<< std::endl;
				}
				if (log_)
				{
					cb_(cyng::generate_invoke("sml.log", ss.str()));
				}
			}

			push(cyng::make_object(std::move(octet)));
		}

		void parser::push(cyng::object obj)
		{
			if (!stack_.empty())
			{
				if (stack_.top().target_ < stack_.top().values_.size())	{
					if (verbose_ || log_) {
						std::stringstream ss;
						ss
						<< std::dec
						<< "list ["
						<< stack_.size()
						<< "] is full with "
						<< stack_.top().values_.size()
						<< " entries - <"
						<< cyng::io::to_str(obj)
						<< "> exceeds expected size "
						<< stack_.top().target_
						;
					}
				}
				
				if (stack_.top().push(obj))
				{
					//
					//	list is complete
					//
					auto values = stack_.top().values_;
					if (verbose_ || log_) {
						std::stringstream ss;
						ss 
							<< "reduce list ["
							<< std::dec
							<< stack_.size()
							<< "] "
							<< values.size()
							;
						
						if (verbose_)
						{
							std::cerr
								<< ss.str()
								<< std::endl;
						}
						if (log_)
						{
							cb_(cyng::generate_invoke("sml.log", ss.str()));
						}
					}
					stack_.pop();
					//	recursive
					push(cyng::make_object(values));
				}
			}
			else //if (verbose_)
			{
				std::stringstream ss;
				ss
					<< "SML protocol error (cannot reduce list) @"
					<< std::hex
					<< pos_
					;
				cb_(cyng::generate_invoke("log.msg.fatal", ss.str()));
			}
		}

		void parser::finalize(std::uint16_t crc, std::uint8_t gap)
		{
			cb_(cyng::generate_invoke("sml.eom", this->crc_, counter_));
			counter_ = 0;
			pos_ = 0;
		}

		std::string parser::prefix() const
		{
			std::stringstream ss;
			if (verbose_)
			{
				if (!stack_.empty())
				{
					ss << "[";

					for (std::size_t indent = 0; indent < stack_.size(); indent++)
					{
						ss << "....";
					}
					ss
						<< stack_.size()
						//<< stack_.top().counter()
						<< "] "
						;
				}
				else
				{
					ss
						<< "[START] "
						;
				}
			}
			return ss.str();
		}

		parser::list::list(std::size_t size)
			: target_(size)
			, values_()
		{}

		bool parser::list::push(cyng::object obj)
		{
			BOOST_ASSERT(values_.size() < target_);
			values_.push_back(obj);
			return values_.size() == target_;
		}

	}
}	//	node

