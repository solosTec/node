/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/iec/parser.h>

#include <cyng/parse/string.h>

#include <iostream>

#include <boost/assert.hpp>
#include <boost/algorithm/string.hpp>

namespace smf
{
	namespace iec
	{
		tokenizer::tokenizer(cb_f cb, cb_final_f cbf)
			: state_(state::START)
			, cb_(cb)
			, final_(cbf)
			, id_()
			, data_()
			, bbc_(0)
			, crc_(0)
		{}

		void tokenizer::put(char c) {

			switch (state_) {
			case state::START:
				//	expect '/'
				state_ = state_start(c);
				break;
			case state::DEVICE_ID:
				state_ = state_device_id(c);
				break;
			case state::DEVICE_ID_COMPLETE:
				BOOST_ASSERT(LF == c);
				state_ = (LF == c)
					? state::STX
					: state::START
					;
				break;
			case state::STX:
				//BOOST_ASSERT(STX == c);
				state_ = (STX == c)
					? state::DATA
					: state_bbc(c)	//	error
					;
				break;
			case state::DATA:
				crc_ ^= c;
				state_ = state_data(c);
				break;
			case state::DATA_LINE:
				BOOST_ASSERT(LF == c);
				crc_ ^= c;
				state_ = (LF == c)
					? state::DATA
					: state::START
					;
				break;
			case state::DATA_COMPLETE:
				BOOST_ASSERT(CR == c);
				crc_ ^= c;
				state_ = (CR == c)
					? state::DATA_COMPLETE_LF
					: state::START
					;
				break;
			case state::DATA_COMPLETE_LF:
				BOOST_ASSERT(LF == c);
				crc_ ^= c;
				state_ = (LF == c)
					? state::ETX
					: state::START
					;
				break;
			case state::ETX:
				BOOST_ASSERT(ETX == c);
				state_ = (ETX == c)
					? state::BBC
					: state::START
					;
				break;
			case state::BBC:
				state_ = state_bbc(c);
				break;
			default:
				BOOST_ASSERT_MSG(false, "illegal parser state");
				break;
			}
		}

		tokenizer::state tokenizer::state_start(char c) {
			id_.clear();
			data_.clear();
			bbc_ = 0;
			BOOST_ASSERT(start_c == c);
			if (start_c == c)	return state::DEVICE_ID;
			return state_;	//	wait for valid character
		}

		tokenizer::state tokenizer::state_device_id(char c) {
			if (CR == c)	return state::DEVICE_ID_COMPLETE;
			id_.push_back(c);
			BOOST_ASSERT(id_.size() < 60);
			return state_;
		}

		tokenizer::state tokenizer::state_data(char c) {
			if (end_c == c)	return state::DATA_COMPLETE;
			if (CR == c) {
				cb_(data_);
				data_.clear();
				return  state::DATA_LINE;
			}
			data_.push_back(c);
			return state_;
		}

		tokenizer::state tokenizer::state_bbc(char c) {
			bbc_ = c;
			//BOOST_ASSERT(crc_ == bbc_);
			boost::algorithm::trim_right(id_);
			final_(id_, crc_ == bbc_);
			return state::START;
		}


		std::string tokenizer::get_device_id() const {
			return id_;
		}

		std::uint8_t tokenizer::get_bbc() const {
			return bbc_;
		}

		/**
		 *	IEC 62056-21 parser (readout mode).
		 */
		parser::parser(cb_data_f cbd, cb_final_f cbf, std::uint8_t medium)
			: tokenizer_([this](std::string line) {
				std::cout << line << std::endl;
				auto const r = this->split(line);
				if (r.size() == 2) {
					convert(r.at(0), r.at(1));
				}
				else if (r.size() == 3) {
					convert(r.at(0), r.at(1), r.at(2));
				}
				BOOST_ASSERT_MSG(r.size() < 4, "unknown EDIS format");
			}, cbf)
			, data_(cbd)
			, medium_(medium)
		{}

		std::vector<std::string> parser::split(std::string const& line) {
			std::vector<std::string> r{ "" };

			std::size_t parity = 0;
			BOOST_ASSERT(!line.empty());
			for (auto const c : line) {
				switch (c) {
				case '(':
					r.push_back("");
					++parity;
					break;
				case ')':
					--parity;
					BOOST_ASSERT(parity == 0);
					break;
				default:
					BOOST_ASSERT(!r.empty());
					r.back().push_back(c);
					break;
				}
			}
			BOOST_ASSERT(parity == 0);
			return r;
		}

		void parser::convert(std::string const& code, std::string const& value) {
			auto const c = to_obis(code, medium_);
			auto const [v, unit] = split_edis_value(value);
			data_(c, v, unit);
		}
		void parser::convert(std::string const& code, std::string const& index, std::string const& value) {
			BOOST_ASSERT(index.size() == 2);
			auto const c = to_obis(code, medium_);
			auto const [v, unit] = split_edis_value(value);
			data_(c, v, unit);
		}

		cyng::obis to_obis(std::string code, std::uint8_t medium) {

			auto const vec = cyng::split(code, ".");

			BOOST_ASSERT(!vec.empty());
			BOOST_ASSERT(vec.size() < 4);

			if (vec.size() == 2) {

				//
				//	handle some special cases first
				//
				if (boost::algorithm::equals(code, "F.F")) {
					return cyng::make_obis(0, 0, 0x61, 0x61, 0, 0xFF);
				}

				//
				//	"P.nn" set profile 
				//
			}
			else if (vec.size() == 3)	{

				//
				//	handle some more special cases
				// F.F.n
				//
				if (boost::algorithm::starts_with(code, "F.F")) {
					try {
						auto const e = static_cast<std::int8_t>(std::stoi(vec.at(2)));
						return cyng::make_obis(0, 0, 0x61, 0x61, e, 0xFF);
					}
					catch (std::exception const&) {}
					return cyng::make_obis(0, 0, 0x61, 0x61, 0, 0xFF);
				}

				//	"A.n.n" historical data
				//	"B.n.n" historical data
				//	"C.n.n" historical data (count)
				//	"D.n.n" historical data
				//	"E.n.n" historical data
				if (boost::algorithm::starts_with(code, "A")
					|| boost::algorithm::starts_with(code, "B")
					|| boost::algorithm::starts_with(code, "C")
					|| boost::algorithm::starts_with(code, "D")
					|| boost::algorithm::starts_with(code, "E")) {

					//
					//	not supported
					//
					try {
						auto const d = static_cast<std::int8_t>(std::stoi(vec.at(1)));
						auto const e = static_cast<std::int8_t>(std::stoi(vec.at(2)));

						return cyng::make_obis(medium, 0, 0, d, e, 0xFF);
					}
					catch (std::exception const&) {}
				}
				else {

					//
					//	default case
					//
					auto const [c, d, e] = to_u8(vec.at(0), vec.at(1), vec.at(2));
					return (c == 0)
						? cyng::make_obis(0, 0, c, d, e, 0xFF)
						: cyng::make_obis(medium, 0, c, d, e, 0xFF)
						;
				}
			}
			return cyng::obis();
		}

		std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> to_u8(std::string c, std::string d, std::string e) {
			try {
				return {
					static_cast<std::int8_t>(std::stoi(c)),
					static_cast<std::int8_t>(std::stoi(d)),
					static_cast<std::int8_t>(std::stoi(e))
				};
			}
			catch (...) {}
			return { 0, 0, 0 };
		}

		std::tuple<std::string, std::string> split_edis_value(std::string value) {
			auto const vec = cyng::split(value, "*");
			if (vec.size() == 2) {
				return { vec.at(0), vec.at(1) };
			}
			return { value, "" };
		}

	}
}


