/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/iec/parser.h>

#include <cyng/parse/string.h>

#include <iostream>
#include <regex>

#include <boost/algorithm/string.hpp>
#include <boost/assert.hpp>

namespace smf {
    namespace iec {
        tokenizer::tokenizer(cb_f cb, cb_final_f cbf)
            : state_(state::START)
            , cb_(cb)
            , final_(cbf)
            , id_()
            , data_()
            , bbc_(0)
            , crc_(0) {}

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
                state_ = (LF == c) ? state::STX : state::START;
                break;
            case state::STX:
                // BOOST_ASSERT(STX == c);
                state_ = (STX == c) ? state::DATA : state_bbc(c) //	error
                    ;
                break;
            case state::DATA:
                crc_ ^= c;
                state_ = state_data(c);
                break;
            case state::DATA_LINE:
                BOOST_ASSERT(LF == c);
                crc_ ^= c;
                state_ = (LF == c) ? state::DATA : state::START;
                break;
            case state::DATA_COMPLETE:
                BOOST_ASSERT(CR == c);
                crc_ ^= c;
                state_ = (CR == c) ? state::DATA_COMPLETE_LF : state::START;
                break;
            case state::DATA_COMPLETE_LF:
                BOOST_ASSERT(LF == c);
                crc_ ^= c;
                state_ = (LF == c) ? state::ETX : state::START;
                break;
            case state::ETX:
                BOOST_ASSERT(ETX == c);
                state_ = (ETX == c) ? state::BBC : state::START;
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
            // BOOST_ASSERT(start_c == c);
            if (start_c == c)
                return state::DEVICE_ID;
            return state_; //	wait for valid character
        }

        tokenizer::state tokenizer::state_device_id(char c) {
            if (CR == c)
                return state::DEVICE_ID_COMPLETE;
            id_.push_back(c);
            BOOST_ASSERT(id_.size() < 60);
            return state_;
        }

        tokenizer::state tokenizer::state_data(char c) {
            if (end_c == c)
                return state::DATA_COMPLETE;
            if (CR == c) {
                cb_(data_);
                data_.clear();
                return state::DATA_LINE;
            }
            data_.push_back(c);
            return state_;
        }

        tokenizer::state tokenizer::state_bbc(char c) {
            bbc_ = c;
            // BOOST_ASSERT(crc_ == bbc_);
            boost::algorithm::trim_right(id_);
            final_(id_, crc_ == bbc_);
            return state::START;
        }

        std::string tokenizer::get_device_id() const { return id_; }

        std::uint8_t tokenizer::get_bbc() const { return bbc_; }

        /**
         *	IEC 62056-21 parser (readout mode).
         */
        parser::parser(cb_data_f cbd, cb_final_f cbf, std::uint8_t medium)
            : tokenizer_(
                  [this](std::string line) {
                      // std::cout << line << std::endl;
                      auto const r = split_line(line);
                      if (r.size() == 2) {
                          convert(r.at(0), r.at(1));
                      } else if (r.size() == 3) {
                          convert(r.at(0), r.at(1), r.at(2));
                      }
                      BOOST_ASSERT_MSG(r.size() < 4, "unknown EDIS format");
                  },
                  cbf)
            , cb_data_(cbd)
            , medium_(medium) {
            BOOST_ASSERT(cb_data_);
        }

        std::vector<std::string> split_line(std::string const &line) {
            std::vector<std::string> r{""};

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

        void parser::convert(std::string const &code, std::string const &value) {
            auto const c = to_obis(code, medium_);
            auto const [v, unit] = split_edis_value(value);
            cb_data_(c, v, unit);
        }
        void parser::convert(std::string const &code, std::string const &index, std::string const &value) {
            // BOOST_ASSERT(index.size() == 2);
            if (index.size() == 2) {
                //  example: 1.4.0(04)(00.000*kW)
                auto const c = to_obis(code, medium_);
                auto const [v, unit] = split_edis_value(value);
                cb_data_(c, v, unit);
            } else {
                // std::cout << code << ", " << index << ", " << value << std::endl;
                if (contains(index, '*')) {
                    //  example: 1.6.1(02.67*kW)(2105081945)
                    //  ignore profile data
                } else if (contains(index, '.')) {
                    //  example: 1.6.1*90(02.79)(2104231930)
                    //  ignore profile data
                }
            }
        }

        cyng::obis to_obis(std::string code, std::uint8_t medium) {

            // std::regex delimiter{ "[\\(\\)]" };
            // std::vector<std::string> vec(std::sregex_token_iterator(str.begin(), str.end(), delimiter, -1), {});

            // BOOST_ASSERT(!vec.empty());
            // BOOST_ASSERT(vec.size() < 4);

            // if (vec.size() > 1) {

            //	//
            //	//	extract the edis code
            //	//
            //	auto const code = vec.at(0);

            //
            //	handle some special cases first
            //
            if (boost::algorithm::equals(code, "F.F")) {
                return cyng::make_obis(0, 0, 0x61, 0x61, 0, 0xFF);
            }

            //
            //	split code
            //
            std::regex delimiter{"[\\.\\*]"};
            std::vector<std::string> edis_code(std::sregex_token_iterator(code.begin(), code.end(), delimiter, -1), {});
            BOOST_ASSERT(!edis_code.empty());
            BOOST_ASSERT(edis_code.size() < 5);

            if (edis_code.size() == 3) {

                //
                //	handle some more special cases
                // F.F.n
                //
                if (boost::algorithm::starts_with(code, "F.F")) {
                    try {
                        auto const e = static_cast<std::int8_t>(std::stoi(edis_code.at(2)));
                        return cyng::make_obis(0, 0, 0x61, 0x61, e, 0xFF);
                    } catch (std::exception const &) {
                    }
                    return cyng::make_obis(0, 0, 0x61, 0x61, 0, 0xFF);
                }

                if (boost::algorithm::starts_with(code, "C.")) {
                    //	C => 60hex
                    //	C.1.1		=> 00 00 60 01 01 FF
                    //	C.2.1		=> Date of last parameterisation (00-03-26)
                    //	C.7.0		=> 00 00 60 07 00 FF	- Number of power outages
                    //  C.240.13	=> 00 00 60 F0 0D FF	- hardware type
                    //	C.3.0		=> Active pulse constant (500)
                    //	C.3.1		=> Reactive pulse constant (500)
                    //
                    try {
                        auto const d = static_cast<std::int8_t>(std::stoi(edis_code.at(1)));
                        auto const e = static_cast<std::int8_t>(std::stoi(edis_code.at(2)));

                        return cyng::make_obis(0, 0, 0x60, d, e, 0xFF);
                    } catch (std::exception const &) {
                    }
                }

                if (boost::algorithm::starts_with(code, "D.")) {
                    //	D => 62hex
                    //	D.23.0		=> 01 01 62 17 00 FF
                    //
                    try {
                        auto const d = static_cast<std::int8_t>(std::stoi(edis_code.at(1)));
                        auto const e = static_cast<std::int8_t>(std::stoi(edis_code.at(2)));

                        return cyng::make_obis(1, 0, 0x62, d, e, 0xFF);
                    } catch (std::exception const &) {
                    }
                }

                //	"A.n.n" historical data
                //	"B.n.n" historical data
                //	"E.n.n" historical data
                if (boost::algorithm::starts_with(code, "A.") || boost::algorithm::starts_with(code, "B.") ||
                    boost::algorithm::starts_with(code, "E.")) {

                    //
                    //	not supported
                    //
                    try {
                        auto const d = static_cast<std::int8_t>(std::stoi(edis_code.at(1)));
                        auto const e = static_cast<std::int8_t>(std::stoi(edis_code.at(2)));

                        return cyng::make_obis(medium, 0, 0, d, e, 0xFF);
                    } catch (std::exception const &) {
                    }
                } else {

                    //
                    //	the common case
                    //
                    auto const [c, d, e] = to_u8(edis_code.at(0), edis_code.at(1), edis_code.at(2));
                    return (c == 0) ? cyng::make_obis(0, 0, c, d, e, 0xFF) : cyng::make_obis(medium, 0, c, d, e, 0xFF);
                }

            } else if (edis_code.size() == 4) {

                if (boost::algorithm::starts_with(code, "L.")) {
                    //	L.1.0*126		=> Vorwerte
                    //
                    try {
                        auto const d = static_cast<std::int8_t>(std::stoi(edis_code.at(1)));
                        auto const e = static_cast<std::int8_t>(std::stoi(edis_code.at(2)));

                        return cyng::make_obis(0, 0, 0x62, d, e, 126);
                    } catch (std::exception const &) {
                    }
                }

                //
                //	example: 2.8.0*88(00000.03)
                //
                auto const [c, d, e, f] = to_u8(edis_code.at(0), edis_code.at(1), edis_code.at(2), edis_code.at(3));
                return (c == 0) ? cyng::make_obis(0, 0, c, d, e, f) : cyng::make_obis(medium, 0, c, d, e, f);
            }
            //}
            return cyng::obis();
        }

        std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> to_u8(std::string c, std::string d, std::string e) {
            try {
                return {
                    static_cast<std::int8_t>(std::stoi(c)),
                    static_cast<std::int8_t>(std::stoi(d)),
                    static_cast<std::int8_t>(std::stoi(e))};
            } catch (...) {
            }
            return {0, 0, 0};
        }

        std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t>
        to_u8(std::string c, std::string d, std::string e, std::string f) {
            try {
                return {
                    static_cast<std::int8_t>(std::stoi(c)),
                    static_cast<std::int8_t>(std::stoi(d)),
                    static_cast<std::int8_t>(std::stoi(e)),
                    static_cast<std::int8_t>(std::stoi(f))};
            } catch (...) {
            }
            return {0, 0, 0, 0};
        }

        std::tuple<std::string, std::string> split_edis_value(std::string value) {
            auto const vec = cyng::split(value, "*");
            if (vec.size() == 2) {
                return {vec.at(0), vec.at(1)};
            }
            return {value, ""};
        }

        bool contains(std::string const &inp, char c) { return inp.find(c, 0) != std::string::npos; }

    } // namespace iec
} // namespace smf
