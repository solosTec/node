#include <smf/imega/parser.h>

#include <cyng/parse/string.h>
#include <cyng/parse/version.h>

#include <ios>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/core/ignore_unused.hpp>

namespace smf {
    namespace imega {
        parser::parser(cb_login login, cb_watchdog watchdog, cb_data data)
            : state_(state::INITIAL)
            , cb_login_(login)
            , cb_watchdog_(watchdog)
            , cb_data_(data)
            , buffer_() {
            buffer_.reserve(64);
        }

        void parser::reset() {
            state_ = state::INITIAL;
            buffer_.clear();
            buffer_.reserve(64);
        }

        void parser::put(char c) {
            switch (state_) {
            case state::INITIAL:
                //	initial - expect a '<'
                state_ = state_initial(c);
                break;
            case state::LOGIN: //	gathering login data
                state_ = state_login(c);
                break;
            case state::STREAM_MODE:
                //	parser in data mode
                state_ = state_stream_mode(c);
                break;
            case state::ALIVE_A:
                //	probe <ALIVE> watchdog
                state_ = state_alive_A(c);
                break;
            case state::ALIVE_L:
                //	probe <ALIVE> watchdog
                state_ = state_alive_L(c);
                break;
            case state::ALIVE_I:
                //	probe <ALIVE> watchdog
                state_ = state_alive_I(c);
                break;
            case state::ALIVE_V:
                //	probe <ALIVE> watchdog
                state_ = state_alive_V(c);
                break;
            case state::ALIVE_E:
                //	probe <ALIVE> watchdog
                state_ = state_alive_E(c);
                break;
            case state::ALIVE:
                //	until '>'
                state_ = state_alive(c);
                break;
            case state::ERROR_: //!>	error
            default:
                //  error recovery
                cb_data_(std::move(buffer_), false);
                state_ = state::STREAM_MODE;
                break;
            }
        }

        bool parser::post_processing() {
            if (state_ == state::STREAM_MODE && !buffer_.empty()) {
                cb_data_(std::move(buffer_), true);
                return true;
            }
            return false;
        }

        parser::state parser::state_initial(char c) {
            //
            //	expect something like:
            //	<PROT:1;VERS:1.0;TELNB:96110845-2;MNAME:95298073>
            //	TELNB ist the login name (96110845-2)
            //
            return (c == '<') ? state::LOGIN : state::ERROR_;
        }

        parser::state parser::state_login(char c) {
            if (c == '>') {

                std::string s(buffer_.begin(), buffer_.end());
                auto const vec = cyng::split(s, ";");
                if (vec.size() == 4) {
                    //	example: <PROT:1;VERS:1.0;TELNB:96110845-2;MNAME:95298073>
                    //  (std::uint8_t, cyng::version, std::string, std::string)
                    std::map<std::string, std::string> data;
                    std::transform(vec.begin(), vec.end(), std::inserter(data, data.end()), [](std::string s) {
                        auto const kv = cyng::split(s, ":");
                        return (kv.size() == 2) ? std::map<std::string, std::string>::value_type{kv.at(0), kv.at(1)}
                                                : std::map<std::string, std::string>::value_type{s, s};
                    });

                    cb_login_(get_protocol(data), get_version(data), get_modulename(data), get_meterid(data));
                    buffer_.clear();

                    //
                    //   complete - goto stream mode
                    //
                    return state::STREAM_MODE;
                }
                //  error state
                cb_data_(std::move(buffer_), false);
                return state::INITIAL;
            }

            //
            //  add to buffer
            //
            buffer_.push_back(c);
            return state_;
        }

        parser::state parser::state_stream_mode(char c) {
            if (c == '<') {
                //  expect an 'A'
                return state::ALIVE_A;
            }
            buffer_.push_back(c);
            return state_;
        }

        parser::state parser::state_alive_A(char c) {
            if (c == 'A') {
                //  expect an 'L'
                return state::ALIVE_L;
            }
            buffer_.push_back('<');
            buffer_.push_back(c);
            return state::STREAM_MODE;
        }

        parser::state parser::state_alive_L(char c) {
            if (c == 'L') {
                //  expect an 'L'
                return state::ALIVE_I;
            }
            buffer_.push_back('<');
            buffer_.push_back('A');
            buffer_.push_back(c);
            return state::STREAM_MODE;
        }

        parser::state parser::state_alive_I(char c) {
            if (c == 'I') {
                //  expect an 'V'
                return state::ALIVE_V;
            }
            buffer_.push_back('<');
            buffer_.push_back('A');
            buffer_.push_back('L');
            buffer_.push_back(c);
            return state::STREAM_MODE;
        }

        parser::state parser::state_alive_V(char c) {
            if (c == 'V') {
                //  expect an 'E'
                return state::ALIVE_E;
            }
            buffer_.push_back('<');
            buffer_.push_back('A');
            buffer_.push_back('L');
            buffer_.push_back('I');
            buffer_.push_back(c);
            return state::STREAM_MODE;
        }
        parser::state parser::state_alive_E(char c) {
            if (c == 'E') {
                //  expect an '>'
                return state::ALIVE;
            }
            buffer_.push_back('<');
            buffer_.push_back('A');
            buffer_.push_back('L');
            buffer_.push_back('I');
            buffer_.push_back('V');
            buffer_.push_back(c);
            return state::STREAM_MODE;
        }
        parser::state parser::state_alive(char c) {
            if (c == '>') {
                //  complete
                cb_watchdog_();
                return state::STREAM_MODE;
            }
            buffer_.push_back('<');
            buffer_.push_back('A');
            buffer_.push_back('L');
            buffer_.push_back('I');
            buffer_.push_back('V');
            buffer_.push_back('E');
            buffer_.push_back(c);
            return state::STREAM_MODE;
        }

        std::uint8_t get_protocol(std::map<std::string, std::string> const &p) {
            //	<PROT:1;VERS:1.0;TELNB:96110845-2;MNAME:95298073>
            auto pos = p.find("PROT");
            return (pos != p.end()) ? cyng::to_numeric<std::uint8_t>(pos->second) : 0;
        }
        cyng::version get_version(std::map<std::string, std::string> const &p) {
            auto pos = p.find("VERS");
            return (pos != p.end()) ? cyng::to_version(pos->second) : cyng::version(0, 0);
        }
        std::string get_modulename(std::map<std::string, std::string> const &p) {
            auto pos = p.find("TELNB");
            return (pos != p.end()) ? pos->second : "";
        }
        std::string get_meterid(std::map<std::string, std::string> const &p) {
            auto pos = p.find("MNAME");
            return (pos != p.end()) ? pos->second : "";
        }

    } // namespace imega
} // namespace smf
