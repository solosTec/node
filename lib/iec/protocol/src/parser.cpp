/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/iec/parser.h>
#include <smf/iec/defs.h>
#include <smf/sml/obis_io.h>

#include <cyng/vm/generator.h>
#include <boost/algorithm/string.hpp>

namespace node	
{
	namespace iec
	{
		/*
		 *	IEC 62056-21 parser.
		 */
		parser::parser(parser_callback cb, bool verbose)
			: cb_(cb)
			, verbose_(verbose)
			, code_()
			, bcc_(0)
			, state_(STATE_START)
			, parser_state_(iec_start())
			, id_()
		{
			BOOST_ASSERT_MSG(cb_, "no callback specified");
		}

		parser::~parser()
		{}

		void parser::put(char c)
		{
			auto prev_state = state_;
			state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
			if (state_ != prev_state) {
				switch (state_) {
				case STATE_ADDRESS_A:
					parser_state_ = iec_address_a();
					break;
				case STATE_ADDRESS_D:
					parser_state_ = iec_address_d();
					break;
				case STATE_ADDRESS_E:
					parser_state_ = iec_address_e();
					break;
				case STATE_ADDRESS_F:
					parser_state_ = iec_address_f();
					break;
				case STATE_CHOICE:
					parser_state_ = iec_choice();
					break;
				case STATE_VALUE:
					parser_state_ = iec_value();
					break;
				case STATE_DATA_LINE:
					parser_state_ = iec_data_line();
					break;
				case STATE_NO_VALUE:
					parser_state_ = iec_no_value();
					break;
				case STATE_UNIT:
					parser_state_ = iec_unit();
					break;
				default:
					break;
				}
			}
		}


		void parser::post_processing()	
		{
		}

		void parser::update_bcc(char c)
		{
			bcc_ ^= c;
		}

		void parser::set_value_group(std::size_t idx, std::uint8_t v)
		{
			auto buffer = id_.to_buffer();
			buffer.at(idx) = v;	//	could throw
			id_ = node::sml::obis(buffer);
		}

		void parser::set_value(std::string const& val)
		{
			value_ = val;
			if (verbose_) {
				std::cerr
					<< "IEC: "
					<< node::sml::to_string(id_)
					<< " = "
					<< value_
					<< std::endl;
			}
		}

		void parser::set_unit(std::string const& val)
		{
			if (verbose_) {
				std::cerr
					<< "IEC: unit = "
					<< val
					<< std::endl;
			}
		}


		parser::state_visitor::state_visitor(parser& p, char c)
			: parser_(p)
			, c_(c)
		{}


		parser::state parser::state_visitor::operator()(iec_start& s) const
		{
			if (c_ == STX) {
				this->parser_.id_.clear();
				return STATE_ADDRESS_A;
			}
			return STATE_ERROR;
		}

		parser::state parser::state_visitor::operator()(iec_address_a& s) const
		{
			this->parser_.update_bcc(this->c_);

			//	fields A, B, E, F are optional
			//	fields C & D are mandatory
			switch (this->c_) {
			case '0':	//	Abstract objects
				this->parser_.id_ = node::sml::obis(0, 0, 0, 0, 0, 0);
				return STATE_CHOICE;
			case '1':	//	Electricity related objects
				this->parser_.id_ = node::sml::obis(1, 0, 0, 0, 0, 0);
				return STATE_CHOICE;
			case '4':	//	Heat cost allocator related objects
				this->parser_.id_ = node::sml::obis(4, 0, 0, 0, 0, 0);
				return STATE_CHOICE;
			case '5':	//	Cooling related objects
				this->parser_.id_ = node::sml::obis(5, 0, 0, 0, 0, 0);
				return STATE_CHOICE;
			case '6':	//	Heat related objects
				this->parser_.id_ = node::sml::obis(6, 0, 0, 0, 0, 0);
				return STATE_CHOICE;
			case '7':	//	Gas related objects
				this->parser_.id_ = node::sml::obis(7, 0, 0, 0, 0, 0);
				return STATE_CHOICE;
			case '8':	//	Cold water related objects
				this->parser_.id_ = node::sml::obis(8, 0, 0, 0, 0, 0);
				return STATE_CHOICE;
			case '9':	//	Hot water related objects
				this->parser_.id_ = node::sml::obis(9, 0, 0, 0, 0, 0);
				return STATE_CHOICE;
			case 'C':	//	General service entries
				this->parser_.id_ = node::sml::obis(0, 0, _C, 0, 0, 0);
				return STATE_CHOICE;
			case 'F':	//	General error message
				this->parser_.id_ = node::sml::obis(0, 0, _F, 0, 0, 0);
				return STATE_CHOICE;
			case 'L':	//	General list objects
				this->parser_.id_ = node::sml::obis(0, 0, _L, 0, 0, 0);
				return STATE_CHOICE;
			case 'P':	//	Abstract data profile
				this->parser_.id_ = node::sml::obis(0, 0, _P, 0, 0, 0);
				return STATE_CHOICE;
			case '(':
				return STATE_VALUE;
			default:
				break;
			}
			return STATE_ERROR;
		}


		parser::state parser::state_visitor::operator()(iec_address_d& s) const
		{
			this->parser_.update_bcc(this->c_);

			switch (this->c_) {
			case 'F':	//	General error message
				this->parser_.set_value_group(3, _F);
				return STATE_ADDRESS_E;
			case '.':
				this->parser_.set_value_group(3, std::stoul(s.value_));
				return STATE_ADDRESS_E;
			default:
				s.value_ += this->c_;
				break;
			}
			return STATE_ADDRESS_D;
		}

		parser::state parser::state_visitor::operator()(iec_address_e& s) const
		{
			this->parser_.update_bcc(this->c_);

			switch (this->c_) {
			case '*':
			case '&':
				this->parser_.set_value_group(4, std::stoul(s.value_));
				return STATE_ADDRESS_F;
			case '(':
				if (!s.value_.empty()) {
					this->parser_.set_value_group(4, std::stoul(s.value_));
				}
				this->parser_.set_value_group(5, 0xFF);
				return STATE_VALUE;
			case CR:
				this->parser_.set_value_group(4, std::stoul(s.value_));
				return STATE_ADDRESS_A;
			default:
				s.value_ += this->c_;
				break;
			}
			return STATE_ADDRESS_E;
		}

		parser::state parser::state_visitor::operator()(iec_address_f& s) const
		{
			this->parser_.update_bcc(this->c_);

			switch (this->c_) {
			case '(':
				this->parser_.set_value_group(5, std::stoul(s.value_));
				return STATE_VALUE;
			case CR:
				this->parser_.set_value_group(5, std::stoul(s.value_));
				return STATE_ADDRESS_A;
			default:
				s.value_ += this->c_;
				break;
			}
			return STATE_ADDRESS_F;
		}

		parser::state parser::state_visitor::operator()(iec_choice& s) const
		{
			this->parser_.update_bcc(this->c_);

			switch (this->c_) {
			case '-':	return STATE_ADDRESS_B;
			case '.':	
				if (this->parser_.id_.get_indicator() < 96) {
					//	move element from index 0 to index 2
					this->parser_.set_value_group(2, this->parser_.id_.get_medium());
					this->parser_.set_value_group(0, 0);
				}
				return STATE_ADDRESS_D;
			default:
				break;
			}
			return STATE_START;
		}

		parser::state parser::state_visitor::operator()(iec_value& s) const
		{
			this->parser_.update_bcc(this->c_);

			switch (this->c_) {
			case ')':
				this->parser_.set_value(s.value_);
				return STATE_NO_VALUE;
			case '*':
				this->parser_.set_value(s.value_);
				return STATE_UNIT;
			case '.':
			default:
				s.value_ += this->c_;
				break;
			}
			return STATE_VALUE;
		}

		parser::state parser::state_visitor::operator()(iec_no_value& s) const
		{
			switch (this->c_) {
			case '(':	return STATE_NEW_VALUE;
			case '!':	return STATE_DATA_BLOCK;
			case CR:	return STATE_DATA_LINE;
			default:
				break;
			}
			return STATE_START;
		}

		parser::state parser::state_visitor::operator()(iec_data_line& s) const
		{
			return (this->c_ == LF)
				? STATE_ADDRESS_A
				: STATE_ERROR
				;
		}

		parser::state parser::state_visitor::operator()(iec_unit& s) const
		{
			if (this->c_ == ')') {
				this->parser_.set_unit(s.value_);
				return STATE_NO_VALUE;
			}
			s.value_ += this->c_;
			return STATE_UNIT;
		}

		parser::state parser::state_visitor::operator()(iec_eof& s) const
		{
			return STATE_START;
		}
	}
}	

