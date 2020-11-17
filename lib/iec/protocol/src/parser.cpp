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
			, state_(state::START)
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

			if (state_ == state::START) {
				cb_(cyng::generate_invoke("iec.data.start", pk_));
			}

			//
			//	formats:
			//	OBIS(data)	- Measurement value
			//	OBIS(data)(time) - Extreme value
			//	OBIS(minutes)(data) - Average value
			//	OBIS(minutes)(data) - Average value
			//	OBIS(data)(Sum of the overrun times)(Number of the overrun times) - Overconsumption

			auto prev_state = state_;
			state_ = boost::apply_visitor(state_visitor(*this, c), parser_state_);
			if (state_ != prev_state) {
				switch (state_) {
				case state::OBIS:
					parser_state_ = iec_obis();
					break;
				case state::VALUE:
					parser_state_ = iec_value();
					break;
				case state::DATA_LINE:
					parser_state_ = iec_data_line();
					break;
				case state::CHOICE_VALUE:
					parser_state_ = iec_choice_value();
					break;
				case state::CHOICE_STATUS:
					parser_state_ = iec_choice_status();
					break;
				case state::UNIT:
					parser_state_ = iec_unit();
					break;
				case state::DATA_BLOCK:
					parser_state_ = iec_data_block();
					break;
				case state::DATA_SET:
					parser_state_ = iec_data_set();
					break;
				case state::ETX:
					parser_state_ = iec_etx();
					break;
				case state::BCC:
					parser_state_ = iec_bcc();
					break;
				case state::START:
					parser_state_ = iec_start();
					break;
				case state::STATUS:
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
					cb_(cyng::generate_invoke("log.msg.debug", "IEC - BBC is OK"));
				}
				else {
					cb_(cyng::generate_invoke("log.msg.warning", "IEC - invalid BCC ", +bcc_, "/", +c));
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
					cb_(cyng::generate_invoke("log.msg.debug", "IEC: OBIS ", this->id_.to_str()));
				}
			}
			else if (verbose_) {
				cb_(cyng::generate_invoke("log.msg.warning", "IEC: parsing OBIS code ", val, " failed"));
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
			
			value_ = val;
			if (val.find('.') == std::string::npos) {
				set_status(val);
			}
			if (verbose_) {
				cb_(cyng::generate_invoke("log.msg.debug", "IEC: ", id_.to_str(), " = ", value_));
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
			//	* h
			//	* min

			//
			unit_ = val;
			if (verbose_) {
				cb_(cyng::generate_invoke("log.msg.debug", "IEC: unit = ", val));
			}

			if (boost::algorithm::equals(unit_, "V")
				|| boost::algorithm::equals(unit_, "kV")

				|| boost::algorithm::equals(unit_, "A")
				|| boost::algorithm::equals(unit_, "kA")

				|| boost::algorithm::equals(unit_, "VA")
				|| boost::algorithm::equals(unit_, "kVA")
				|| boost::algorithm::equals(unit_, "MVA")
				|| boost::algorithm::equals(unit_, "GVA")

				|| boost::algorithm::equals(unit_, "W")
				|| boost::algorithm::equals(unit_, "kW")
				|| boost::algorithm::equals(unit_, "MW")
				|| boost::algorithm::equals(unit_, "GW")

				|| boost::algorithm::equals(unit_, "VAh")
				|| boost::algorithm::equals(unit_, "kVAh")
				|| boost::algorithm::equals(unit_, "MVAh")
				|| boost::algorithm::equals(unit_, "GVAh")

				|| boost::algorithm::equals(unit_, "var")
				|| boost::algorithm::equals(unit_, "kvar")
				|| boost::algorithm::equals(unit_, "Mvar")
				|| boost::algorithm::equals(unit_, "Gvar")

				|| boost::algorithm::equals(unit_, "varh")
				|| boost::algorithm::equals(unit_, "kvarh")
				|| boost::algorithm::equals(unit_, "Mvarh")
				|| boost::algorithm::equals(unit_, "Gvarh")

				|| boost::algorithm::equals(unit_, "Wh")
				|| boost::algorithm::equals(unit_, "kWh")
				|| boost::algorithm::equals(unit_, "MWh")
				|| boost::algorithm::equals(unit_, "GWh")
				) {

				id_[sml::obis::VG_MEDIUM] = 1;
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
				return state::OBIS;
			}
			return state::_ERROR;
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
				return state::VALUE;
			case CR:
				return state::OBIS;
			case '!':	
				//
				//	produce code
				//
				this->parser_.cb_(cyng::generate_invoke("iec.data.eof", this->parser_.pk_, this->parser_.counter_));
				this->parser_.reset();
				return state::DATA_BLOCK;
			default:
				s.value_ += this->c_;
				break;
			}
			return state::_ERROR;
		}

		parser::state parser::state_visitor::operator()(iec_value& s) const
		{
			switch (this->c_) {
			case ')':
				this->parser_.set_value(s.value_);
				return state::CHOICE_VALUE;
			case '*':
				this->parser_.set_value(s.value_);
				return state::UNIT;
			case '.':
			default:
				s.value_ += this->c_;
				break;
			}
			return state::VALUE;
		}

		parser::state parser::state_visitor::operator()(iec_choice_value& s) const
		{
			switch (this->c_) {
			case '(':	return state::STATUS;
			case CR:	return state::DATA_LINE;
			default:
				break;
			}
			return state::START;
		}

		parser::state parser::state_visitor::operator()(iec_choice_status& s) const
		{
			return (this->c_ == CR)
				? state::DATA_LINE
				: state::_ERROR
				;
		}

		parser::state parser::state_visitor::operator()(iec_status& s) const
		{
			if (this->c_ == ')') {
				//
				//	set status
				//
				this->parser_.set_status(s.value_);
				return state::CHOICE_STATUS;
			}
			s.value_ += this->c_;
			return state::STATUS;
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
				return state::OBIS;
			}
			return state::_ERROR;
		}

		parser::state parser::state_visitor::operator()(iec_unit& s) const
		{
			if (this->c_ == ')') {
				this->parser_.set_unit(s.value_);
				return state::CHOICE_VALUE;
			}
			s.value_ += this->c_;
			return state::UNIT;
		}

		parser::state parser::state_visitor::operator()(iec_data_block& s) const
		{
			return (this->c_ == CR)
				? state::DATA_SET
				: state::_ERROR
				;
		}

		parser::state parser::state_visitor::operator()(iec_data_set&) const
		{
			return (this->c_ == LF)
				? state::ETX
				: state::_ERROR
				;
		}

		parser::state parser::state_visitor::operator()(iec_etx&) const
		{
			this->parser_.cb_(cyng::generate_invoke("iec.data.bcc", this->parser_.pk_, (this->c_ == ETX)));

			if (this->c_ == ETX) {

				//	ETX is part of BCC
				this->parser_.bcc_flag_ = false;
				return state::BCC;
			}
			return state::_ERROR;
		}

		parser::state parser::state_visitor::operator()(iec_bcc&) const
		{
			this->parser_.test_bcc(this->c_);
			return state::START;
		}
	}
}	

