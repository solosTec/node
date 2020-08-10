/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/sml/protocol/parser.h>
#include <smf/sml/crc16.h>

#include <cyng/set_cast.h>
#include <cyng/factory/set_factory.h>
#include <cyng/vm/generator.h>
#include <cyng/io/hex_dump.hpp>
#include <cyng/io/serializer.h>

#include <iostream>
#include <ios>

#include <boost/uuid/nil_generator.hpp>

namespace node 
{
	namespace sml
	{
//#ifdef _DEBUG
		template <class STACK>
		typename STACK::container_type const& get_container(STACK& a)
		{
			struct hack : STACK {
				static typename STACK::container_type const& get(STACK& a) {
					return a.* & hack::c;
				}
			};
			return hack::get(a);
		}
//#endif

		//
		//	parser
		//
		parser::parser(parser_callback cb
			, bool verbose
			, bool log
			, bool data_only)
		: cb_(cb)
			, verbose_(verbose)
			, log_(log)
			, data_only_(data_only)
			, pk_(boost::uuids::nil_uuid())	//	null UUID
			, pos_(0)
			, code_()
			, tl_()
			, offset_(0)
			, stream_state_(state::START)
			, parser_state_(sml_start())
			, crc_(crc_init())
			, crc_on_(true)
			, counter_(0)
			, stack_()
			, has_frame_(false)
		{
			BOOST_ASSERT_MSG(cb_, "no callback specified");
		}

		parser::parser(parser_callback cb
			, bool verbose
			, bool log
			, bool data_only
			, boost::uuids::uuid pk)
		: cb_(cb)
			, verbose_(verbose)
			, log_(log)
			, data_only_(data_only)
			, pk_(pk)
			, pos_(0)
			, code_()
			, tl_()
			, offset_(0)
			, stream_state_(state::START)
			, parser_state_(sml_start())
			, crc_(crc_init())
			, crc_on_(true)
			, counter_(0)
			, stack_()
			, has_frame_(false)
		{
			BOOST_ASSERT_MSG(cb_, "no callback specified");
		}

		parser::~parser()
		{
			if (cb_)	cb_ = nullptr;
		}


		void parser::reset()
		{
			stream_state_ = state::START;
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

			if (log_)
			{
				std::stringstream ss;

				ss
					<< "SML symbol @"
					<< std::hex << std::setw(2) << std::setfill('0')
					<< pos_
					<< " - "
					<< std::dec << std::setw(2) << std::setfill('0')
					<< +c
					<< "/0x"
					<< std::hex << std::setw(2) << std::setfill('0')
					<< +(c & 0xff)
					<< " <"
					<< state_name()
					<< "> "
					;
				if (!stack_.empty())
				{
					ss
						<< "\t[list "
						<< std::string(stack_.size() - 1, '.')
						<< stack_.top().values_.size()
						<< "/"
						<< stack_.top().target_
						<< "]"
						;

				}
				if (!data_only_)	cb_(cyng::generate_invoke("sml.log", ss.str()));
			}

			auto prev_state = stream_state_;
			stream_state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
			if (prev_state != stream_state_)
			{
				switch (stream_state_)
				{
				case state::START:
					parser_state_ = sml_start();
					break;
				case state::LENGTH:
					parser_state_ = sml_length();
					break;
				case state::LIST:
					parser_state_ = sml_list();
					break;
				case state::STRING:
					parser_state_ = sml_string(tl_.length_ - offset_);
					break;
				case state::BOOLEAN:
					parser_state_ = sml_bool();
					break;
				case state::UINT8:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_uint8(tl_.length_ - offset_);
					break;
				case state::UINT16:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_uint16(tl_.length_ - offset_);
					break;
				case state::UINT32:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_uint32(tl_.length_ - offset_);
					break;
				case state::UINT64:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_uint64(tl_.length_ - offset_);
					break;
				case state::INT8:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_int8(tl_.length_ - offset_);
					break;
				case state::INT16:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_int16(tl_.length_ - offset_);
					break;
				case state::INT32:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_int32(tl_.length_ - offset_);
					break;
				case state::INT64:
					BOOST_ASSERT(offset_ == 1);
					parser_state_ = sml_int64(tl_.length_ - offset_);
					break;
				case state::ESC:
					parser_state_ = sml_esc();
					break;
				case state::PROPERTY:
					parser_state_ = sml_prop();
					break;
				case state::START_STREAM:
					parser_state_ = sml_start_stream();
					break;
				case state::START_BLOCK:
					parser_state_ = sml_start_block();
					break;
				case state::TIMEOUT:
					parser_state_ = sml_timeout();
					break;
				case state::BLOCK_SIZE:
					parser_state_ = sml_block_size();
					break;
				case state::EOM:
					parser_state_ = sml_eom();
					break;

				default:
					if (!data_only_) {
						std::stringstream ss;
						ss
							<< get_state_name(stream_state_)
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

		char const * parser::state_name() const
		{
			switch (stream_state_) {
			case state::PARSE_ERROR:	break;
			case state::START:	return "START";
			case state::LENGTH:	return "LENGTH";
			case state::LIST:	return "LIST";
			case state::STRING:	return "STRING";
			case state::OCTET:	return "OCTECT";
			case state::BOOLEAN:	return "BOOLEAN";
			case state::INT8:	return "INT8";
			case state::INT16:	return "UNT16";
			case state::INT32:	return "INT32";
			case state::INT64:	return "INT64";
			case state::UINT8:	return "UINT8";
			case state::UINT16:	return "UINT16";
			case state::UINT32:	return "UINT32";
			case state::UINT64:	return "UINT64";
			case state::ESC:	return "ESC";
			case state::PROPERTY:	return "PROPERTY";
			case state::START_STREAM:	return "START_STREAM";
			case state::START_BLOCK:	return "START_BLOCK";
			case state::TIMEOUT:	return "TIMEOUT";
			case state::BLOCK_SIZE:  return "BLOCK_SIZE";
			case state::EOM:	return "EOM";
			default:
				break;
			}
			return "ERROR";
		}

		parser::state_visitor::state_visitor(parser& p, char c)
			: parser_(p)
			, c_(c)
		{}

		parser::state parser::state_visitor::operator()(sml_start& s) const
		{
			if (c_ == ESCAPE_SIGN)
			{
				return state::ESC;
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
					if (!parser_.data_only_)
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
					return (SML_LIST == parser_.tl_.type_)
						? state::LIST
						: state::LENGTH
						;
				}
				else
				{
					return parser_.next_state();
				}
			}

			return state::START;
		}

		parser::state parser::state_visitor::operator()(sml_length& s) const
		{
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
		parser::state parser::state_visitor::operator()(sml_list& s) const
		{
			//
			//	calculate length.
			//	4 bits are used to encode the length info.
			//
			parser_.tl_.length_ *= 16;
			parser_.tl_.length_ |= (c_ & 0x0f);
			--parser_.tl_.length_;
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
				return state::START;
			}
			return state::STRING;
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
			if ((parser_.verbose_ || parser_.log_) && !parser_.data_only_)
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
			return state::START;
		}

		parser::state parser::state_visitor::operator()(sml_uint8& s) const
		{
			if (s.push(c_))
			{
				if ((parser_.verbose_ || parser_.log_) && !parser_.data_only_)
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
				return state::START;
			}
			return parser_.stream_state_;
		}
		parser::state parser::state_visitor::operator()(sml_uint16& s) const
		{
			if (s.push(c_))
			{
				if ((parser_.verbose_ || parser_.log_) && !parser_.data_only_)
				{
					std::stringstream ss;
					ss
						<< parser_.prefix()
						<< "UINT16 " 
						<< std::dec
						<< s.u_.n_
						<< "\t0x"
						<< std::hex
						<< s.u_.n_
						;

					if (parser_.stack_.size() == 2) {
						ss
							<< " - "
							<< messages::name_from_value(s.u_.n_)
							;
					}

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
				return state::START;
			}
			return parser_.stream_state_;
		}

		parser::state parser::state_visitor::operator()(sml_uint32& s) const
		{
			if (s.push(c_))
			{
				if ((parser_.verbose_ || parser_.log_) && !parser_.data_only_)
				{
					std::stringstream ss;
					ss
						<< parser_.prefix()
						<< "UINT32 "
						<< std::dec
						<< s.u_.n_
						<< "\t0x"
						<< std::hex
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
				return state::START;
			}
			return parser_.stream_state_;
		}
		parser::state parser::state_visitor::operator()(sml_uint64& s) const
		{
			if (s.push(c_))
			{
				if ((parser_.verbose_ || parser_.log_) && !parser_.data_only_)
				{
					std::stringstream ss;
					ss
						<< parser_.prefix()
						<< "UINT64 "
						<< std::dec
						<< s.u_.n_
						<< "\t0x"
						<< std::hex
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
				return state::START;
			}
			return parser_.stream_state_;
		}
		parser::state parser::state_visitor::operator()(sml_int8& s) const
		{
			if (s.push(c_))
			{
				if ((parser_.verbose_ || parser_.log_) && !parser_.data_only_)
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
				return state::START;
			}
			return parser_.stream_state_;
		}
		parser::state parser::state_visitor::operator()(sml_int16& s) const
		{
			if (s.push(c_))
			{
				if ((parser_.verbose_ || parser_.log_) && !parser_.data_only_)
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
				return state::START;
			}
			return parser_.stream_state_;
		}
		parser::state parser::state_visitor::operator()(sml_int32& s) const
		{
			if (s.push(c_))
			{
				if ((parser_.verbose_ || parser_.log_) && !parser_.data_only_)
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
				return state::START;
			}
			return parser_.stream_state_;
		}
		parser::state parser::state_visitor::operator()(sml_int64& s) const
		{
			if (s.push(c_))
			{
				if ((parser_.verbose_ || parser_.log_) && !parser_.data_only_)
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
				return state::START;
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
					parser_.has_frame_ = true;
					return state::PROPERTY;
				}
			}
			else
			{
				if (parser_.verbose_ && !parser_.data_only_)
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
				return state::START;

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
			case 0x01:	return state::START_STREAM;
			case 0x02:	return state::START_BLOCK;
			case 0x03:	return state::TIMEOUT;
			case 0x04:	return state::BLOCK_SIZE;
			case 0x1a:	return state::EOM;
			default:
				if (!parser_.data_only_) {
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
				return state::START;
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
				return  state::START;
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
				return  state::START;
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
				return  state::START;
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
				return state::START;
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
			case SML_STRING:	return state::STRING;
			case SML_BOOLEAN:	return state::BOOLEAN;
			case SML_INTEGER:
				switch (tl_.length_)
				{
				case 2:	
					return state::INT8;
				case 3:	
					return state::INT16;
				case 4: case 5: 
					return state::INT32;
				case 6: case 7: case 8: case 9:
					return state::INT64;
				default:
					//BOOST_ASSERT_MSG(false, "invalid integer length");
					if (!data_only_)
					{
						std::stringstream ss;
						ss
							<< "invalid length for data type SML_INTEGER "
							<< tl_.length_
							<< " @"
							<< std::hex
							<< pos_
							;
						cb_(cyng::generate_invoke("log.msg.fatal", ss.str()));
					}
					break;
				}
				break;
			case SML_UNSIGNED:	
				switch (tl_.length_)
				{
				case 2:
					return state::UINT8;
				case 3:
					return state::UINT16;
				case 4: case 5:
					return state::UINT32;
				case 6: case 7: case 8: case 9:
					return state::UINT64;
				default:
					//BOOST_ASSERT_MSG(false, "invalid unsigned length");
					if (!data_only_)
					{
						std::stringstream ss;
						ss
							<< "invalid length for data type SML_UNSIGNED "
							<< tl_.length_
							<< " @"
							<< std::hex
							<< pos_
							<< std::dec
							;
						cb_(cyng::generate_invoke("log.msg.fatal", ss.str()));
					}
					break;
				}
				break;
			case SML_LIST:
				//
				//	prepare new list
				//
				if ((verbose_ || log_) && !data_only_) {
					std::stringstream ss;
					ss
						<< "open list with "
						<< std::dec
						<< tl_.length_
						<< " entries @"
						<< std::hex
						<< pos_
						;
					if (stack_.size() == 0 && tl_.length_ == 6) {
						ss << "\t-- new msg";
					}
					else if (stack_.size() == 1 && tl_.length_ == 2) {
						ss << "\t-- msg body";
					}

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

					if (stack_.size() == 0 && tl_.length_ != 6) {
						std::stringstream ss;
						ss
							<< "msg has wrong size: "
							<< tl_.length_
							<< " @"
							<< std::hex
							<< pos_
							;
						cb_(cyng::generate_invoke("log.msg.error", ss.str()));
					}
				}
				if (tl_.length_ == 0) {
					//
					//	empty list as empty tuple
					//
					push(cyng::make_tuple());
				}
				else {
					stack_.push(list(tl_.length_));
				}
				return state::START;
			case SML_EOM:
				if (verbose_)
				{
					std::cerr << prefix()
						<< "unexpected EOM (depth: "
						<< stack_.size()
						<< ")"
						<< std::endl
						;
				}
				return state::START;
			default:
				if (!data_only_) {
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
				if (!data_only_)
				{
					//
					//	generate SML message
					//
					//BOOST_ASSERT_MSG(stack_.top().values_.size() == 5, "sml.msg");
					if (stack_.top().values_.size() == 5) {
						cyng::vector_t prg{ cyng::generate_invoke("sml.msg", stack_.top().values_, counter_, pk_) };

						//
						//	message complete
						//
						cb_(std::move(prg));
					}
					else {
						std::stringstream ss;
						ss
							<< "invalid message size: "
							<< stack_.top().values_.size()
							<< ":\n"
							<< cyng::io::to_type(stack_.top().values_);
							;
						cb_(cyng::generate_invoke("log.msg.error", ss.str()));

					}

					if (verbose_)
					{
						std::cerr
							<< "*** SML message complete"
							<< std::endl;
					}
				}

				//
				//	remove accumulated values from stack
				//
				stack_.pop();
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
						;
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
				if (stack_.top().target_ < stack_.top().values_.size() && !data_only_)	{
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
					cb_(cyng::generate_invoke("log.msg.error", ss.str()));
				}

				//
				//	send only data and position info
				//
				if (data_only_) {
					auto const tpl = cyng::tuple_factory(pos_
						, stack_.size()
						, ((!stack_.empty()) ? stack_.top().values_.size() : 1u)
						, obj);
					cb_(cyng::to_vector(tpl));
				}
				
				if (stack_.top().push(obj))
				{
					//
					//	list is complete
					//
					auto const values = stack_.top().values_;
					if (verbose_ || log_) {
						std::stringstream ss;
						ss 
							<< "reduce list with "
							<< std::dec
							<< std::setw(2)
							<< values.size()
							<< " values [stack size "
							<< std::setw(2)
							<< stack_.size()
							<< "] "
							;
						dump_stack(ss);
						
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
			else
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
			if (data_only_) {
				auto const tpl = cyng::tuple_factory(pos_
					, static_cast<std::size_t>(0u)
					, static_cast<std::size_t>(0u)
					, cyng::make_object(crc));

				cb_(cyng::to_vector(tpl));
			}
			else {
				cb_(cyng::generate_invoke("sml.eom", this->crc_, counter_, pk_));
			}
			counter_ = 0;
			pos_ = 0;
		}

		void parser::dump_stack(std::ostream& os) const
		{
			auto c = get_container(stack_);
			os << "[";
			bool init{ false };
			for (auto const& e : c) {
				if (init) {
					os 
						<< "," 
						<< e.values_.size()
						<< "/"
						<< e.target_
						;
				}
				else {
					init = true;
					os << e.values_.size() - 1;
				}
			}
			os << "] ";
		}

		std::string parser::prefix() const
		{
			std::stringstream ss;
			if (verbose_)
			{
				if (stack_.empty()) {
					ss
						<< "[START] "
						;
				}
				else if (stack_.size() == 1)
				{
					ss 
						<< "[" 
						<< stack_.top().pos() - 1
						<< "] "
						;
				}
				else
				{
					//	get container
					dump_stack(ss);
				}
			}
			return ss.str();
		}

		bool parser::is_msg_complete() const
		{
			if (stack_.size() == 1) {
				if ((stack_.top().target_ == 6) && (stack_.top().values_.size() == 5)) {
					//
					//	pos_ has to be aligned of boundary (mod 4)
					//
					return (has_frame_)
						? ((pos_ + 2) % 4 != 0)
						: true
						;
				}
			}
			return false;
		}

		const char* parser::get_state_name(state e)
		{
			switch (e) {
			case state::PARSE_ERROR:	return "PARSE_ERROR";
			case state::START:	return "START";
			case state::LENGTH:	return "LENGTH";
			case state::STRING:	return "STRING";
			case state::OCTET:	return "OCTET";
			case state::BOOLEAN:	return "BOOLEAN";
			case state::INT8:	return "INT8";
			case state::INT16:	return "INT16";
			case state::INT32:	return "INT32";
			case state::INT64:	return "INT64";
			case state::UINT8:	return "UINT8";
			case state::UINT16:	return "UINT16";
			case state::UINT32:	return "UINT32";
			case state::UINT64:	return "UINT64";
			case state::ESC:	return "ESC";
			case state::PROPERTY:	return "PROPERTY";
			case state::START_STREAM:	return "START_STREAM";
			case state::START_BLOCK:	return "START_BLOCK";
			case state::TIMEOUT:	return "TIMEOUT";
			case state::BLOCK_SIZE:	return "BLOCK_SIZE";
			case state::EOM:	return "EOM";
			default:
				break;
			}
			return "undefined state";
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

		std::size_t parser::list::pos() const
		{
			return target_ - values_.size();
		}

	}
}	//	node

