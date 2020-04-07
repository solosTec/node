/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/iec/parser.h>
#include <smf/iec/defs.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/parser/obis_parser.h>

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
			, bcc_(0)
			, bcc_flag_(false)
			, state_(STATE_START)
			, parser_state_(iec_start())
			, value_()
			, unit_()
			, status_()
			, id_()
			, counter_(0)
			, rgn_()
			, pk_(rgn_())
		{
			BOOST_ASSERT_MSG(cb_, "no callback specified");
		}

		parser::~parser()
		{
			if (cb_)	cb_ = nullptr;
		}

		void parser::put(char c)
		{
			if (bcc_flag_) {
				update_bcc(c);
			}

			if (state_ == STATE_START) {
				cb_(cyng::generate_invoke("iec.data.start", pk_));
			}

			auto prev_state = state_;
			state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
			if (state_ != prev_state) {
				switch (state_) {
				case STATE_OBIS:
					parser_state_ = iec_obis();
					break;
				case STATE_VALUE:
					parser_state_ = iec_value();
					break;
				case STATE_DATA_LINE:
					parser_state_ = iec_data_line();
					break;
				case STATE_CHOICE_VALUE:
					parser_state_ = iec_choice_value();
					break;
				case STATE_CHOICE_STATUS:
					parser_state_ = iec_choice_status();
					break;
				case STATE_UNIT:
					parser_state_ = iec_unit();
					break;
				case STATE_DATA_BLOCK:
					parser_state_ = iec_data_block();
					break;
				case STATE_DATA_SET:
					parser_state_ = iec_data_set();
					break;
				case STATE_ETX:
					parser_state_ = iec_etx();
					break;
				case STATE_BCC:
					parser_state_ = iec_bcc();
					break;
				case STATE_START:
					parser_state_ = iec_start();
					break;
				case STATE_STATUS:
					parser_state_ = iec_status();
					break;
				default:
					break;
				}
			}
		}


		//void parser::post_processing()	
		//{}

		void parser::clear()
		{
			id_.clear();
			value_.clear();
			unit_.clear();
			status_.clear();
		}

		void parser::reset()
		{
			counter_ = 0;
			pk_ = rgn_();
			clear();
		}

		void parser::update_bcc(char c)
		{
			bcc_ ^= c;
		}

		void parser::test_bcc(char c)
		{
			if (verbose_) {
				if (bcc_ == c) {
					std::cerr
						<< "IEC - BBC is OK "
						<< std::endl;
				}
				else {
					std::cerr
						<< "IEC - invalid BCC "
						<< +bcc_
						<< "/"
						<< +c
						<< std::endl;
				}
			}
		}


		void parser::set_id(std::string const& val)
		{
			//
			//	parse OBIS code
			//
			auto const r = node::sml::parse_obis(val);
			if (r.second) {
				this->id_ = r.first;
				if (verbose_) {
					std::cerr
						<< "IEC: OBIS "
						<< this->id_
						<< std::endl;
				}
			}
			else if (verbose_) {
				std::cerr
					<< "IEC: parsing OBIS code "
					<< val
					<< " failed"
					<< std::endl;

			}
		}

		void parser::set_status(std::string const& val)
		{
			//
			//	possible formats are:
			//	* 1180802160000
			//	* 10-02-01 00:15
			//
			//BOOST_ASSERT_MSG(val.size() > 12, "status with invalid length");
			status_ = val;
		}

		void parser::set_value_group(std::size_t idx, std::uint8_t v)
		{
			auto buffer = id_.to_buffer();
			buffer.at(idx) = v;	//	could throw
			id_ = node::sml::obis(buffer);
		}

		void parser::set_value(std::string const& val)
		{
			
			//if (val.size() > 12 && (val.find('*') == std::string::npos)) {
			if (val.find('.') == std::string::npos) {
					set_status(val);
			}
			else {
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
		}

		void parser::set_unit(std::string const& val)
		{
			//
			//	possible values are (see IEC 60027-1):
			//	* V - volts
			//	* A - amperes
			//	* VA - apparent power (Scheinleistung)
			//	* W - active power (Wirkleistung)
			//	* var - reactive power (Blindleistung)
			//	* J - energy
			//	* Wh - dissipative energy
			//
			unit_ = val;
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
				this->parser_.clear();
				this->parser_.bcc_flag_ = true;
				return STATE_OBIS;
			}
			return STATE_ERROR;
		}

		parser::state parser::state_visitor::operator()(iec_obis& s) const
		{
			//	fields A, B, E, F are optional
			//	fields C & D are mandatory
			switch (this->c_) {
			case '(':
				//
				//	parse OBIS code
				//
				this->parser_.set_id(s.value_);
				return STATE_VALUE;
			case CR:
				return STATE_OBIS;
			case '!':	
				//
				//	produce code
				//
				this->parser_.cb_(cyng::generate_invoke("iec.data.eof", this->parser_.pk_, this->parser_.counter_));
				this->parser_.reset();
				return STATE_DATA_BLOCK;
			default:
				s.value_ += this->c_;
				break;
			}
			return STATE_ERROR;
		}

		parser::state parser::state_visitor::operator()(iec_value& s) const
		{
			switch (this->c_) {
			case ')':
				this->parser_.set_value(s.value_);
				return STATE_CHOICE_VALUE;
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

		parser::state parser::state_visitor::operator()(iec_choice_value& s) const
		{
			switch (this->c_) {
			case '(':	return STATE_STATUS;
			case CR:	return STATE_DATA_LINE;
			default:
				break;
			}
			return STATE_START;
		}

		parser::state parser::state_visitor::operator()(iec_choice_status& s) const
		{
			return (this->c_ == CR)
				? STATE_DATA_LINE
				: STATE_ERROR
				;
		}

		parser::state parser::state_visitor::operator()(iec_status& s) const
		{
			if (this->c_ == ')') {
				//
				//	set status
				//
				this->parser_.set_status(s.value_);
				return STATE_CHOICE_STATUS;
			}
			s.value_ += this->c_;
			return STATE_STATUS;
		}

		parser::state parser::state_visitor::operator()(iec_data_line& s) const
		{
			if (this->c_ == LF) {

				//
				//	produce code
				//
				this->parser_.cb_(cyng::generate_invoke("iec.data.line"
					, this->parser_.pk_
					, this->parser_.id_.to_buffer()
					, this->parser_.value_
					, this->parser_.unit_
					, this->parser_.status_
					, this->parser_.counter_));
				++this->parser_.counter_;
				this->parser_.clear();
				return STATE_OBIS;
			}
			return STATE_ERROR;
		}

		parser::state parser::state_visitor::operator()(iec_unit& s) const
		{
			if (this->c_ == ')') {
				this->parser_.set_unit(s.value_);
				return STATE_CHOICE_VALUE;
			}
			s.value_ += this->c_;
			return STATE_UNIT;
		}

		parser::state parser::state_visitor::operator()(iec_data_block& s) const
		{
			return (this->c_ == CR)
				? STATE_DATA_SET
				: STATE_ERROR
				;
		}

		parser::state parser::state_visitor::operator()(iec_data_set&) const
		{
			return (this->c_ == LF)
				? STATE_ETX
				: STATE_ERROR
				;
		}

		parser::state parser::state_visitor::operator()(iec_etx&) const
		{
			this->parser_.cb_(cyng::generate_invoke("iec.data.bcc", this->parser_.pk_, (this->c_ == ETX)));

			if (this->c_ == ETX) {

				//	ETX is part of BCC
				this->parser_.bcc_flag_ = false;
				return STATE_BCC;
			}
			return STATE_ERROR;
		}

		parser::state parser::state_visitor::operator()(iec_bcc&) const
		{
			this->parser_.test_bcc(this->c_);
			return STATE_START;
		}
	}
}	

