/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */
#include <smf/modem/parser.h>
//#include <cyng/vm/generator.h>

#include <ios>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/core/ignore_unused.hpp>

namespace smf {
    namespace modem {

        //
        //	parser
        //
        parser::parser(command_cb cmd_cb, data_cb d_cb, std::chrono::milliseconds guard_time)
            : state_(state::COMMAND)
            , command_cb_(cmd_cb)
            , data_cb_(d_cb)
            , guard_time_(guard_time)
            , cmd_()
            , buffer_()
            , esc_() {
            BOOST_ASSERT_MSG(cmd_cb, "no command callback specified");
            BOOST_ASSERT_MSG(d_cb, "no data callback specified");
        }

        parser::~parser() {}

        bool parser::put(char c) {
            bool advance{true};
            switch (state_) {
            case state::COMMAND:
                std::tie(state_, advance) = state_command(c);
                break;
            case state::DATA:
                std::tie(state_, advance) = state_data(c);
                break;
            case state::STREAM:
                std::tie(state_, advance) = state_stream(c);
                break;
            case state::ESC:
                std::tie(state_, advance) = state_esc(c);
                break;
            default:
                break;
            }

            return advance;
        }

        std::pair<parser::state, bool> parser::state_command(char c) {
            if (is_eol(c)) {
                if (!cmd_.first.empty()) {
                    command_cb_(cmd_.first, cmd_.second);
                    cmd_.first.clear();
                    BOOST_ASSERT_MSG(cmd_.first.empty(), "command data not cleared");
                }
            } else {
                if (c >= 'a' && c <= 'z') {
                    //  uppercase
                    cmd_.first.push_back(c - 32);
                } else if (c >= 'A' && c <= 'Z') {
                    cmd_.first.push_back(c);
                } else if (c == '+') {
                    cmd_.first.push_back(c); //  AT+
                    return {state::DATA, true};
                } else {
                    return {state::DATA, false};
                }
            }
            if (cmd_.first.size() > 2) {
                return {state::DATA, true};
            }
            return {state_, true};
        }

        std::pair<parser::state, bool> parser::state_data(char c) {
            if (is_eol(c)) {
                command_cb_(cmd_.first, cmd_.second);
                cmd_.first.clear();
                cmd_.second.clear();
                return {state::COMMAND, false};
            }
            cmd_.second.push_back(c);
            return {state_, true};
        }

        std::pair<parser::state, bool> parser::state_stream(char c) {
            if (is_esc(c)) {
                //  start timer
                esc_ = esc();
                return {state::ESC, true};
            }
            buffer_.push_back(c);
            return {state_, true};
        }

        std::pair<parser::state, bool> parser::state_esc(char c) {
            if (is_esc(c)) {
                ++esc_;
                if (esc_.is_complete()) {
                    if (esc_.is_on_time(guard_time_)) {
                        if (!buffer_.empty()) {
                            data_cb_(std::move(buffer_));
                            BOOST_ASSERT_MSG(buffer_.empty(), "buffer not cleared");
                            command_cb_("+++", "");
                        }
                        return {state::COMMAND, true};
                    } else {
                        //  timeout
                        buffer_.insert(buffer_.end(), esc_.counter_, '+');
                        return {state_, false};
                    }
                }
                //  need more escape symbols
                return {state_, true};
            }

            //
            //  not an escape symbol - restore data
            //
            buffer_.insert(buffer_.end(), esc_.counter_, '+');
            return {state_, false};
        }

        void parser::post_processing() {
            if (state_ == state::STREAM && !buffer_.empty()) {
                //
                //
                //
                data_cb_(std::move(buffer_));
                BOOST_ASSERT_MSG(buffer_.empty(), "buffer not cleared");
            }
        }

        void parser::set_stream_mode() { state_ = state::STREAM; }

        void parser::set_cmd_mode() {
            post_processing();
            state_ = state::COMMAND;
        }

        void parser::clear() {
            state_ = state::COMMAND;
            buffer_.clear();
        }

        // parser::state_visitor::state_visitor(parser &p, char c)
        //     : parser_(p)
        //     , c_(c) {}

        // parser::state parser::state_visitor::operator()(command &cmd) const {
        //     if (cmd.is_eol(c_)) {
        //         //
        //         //	input is complete
        //         //
        //         std::string str;
        //         std::getline(parser_.input_, str, '\0');

        //        //	clear error flag
        //        parser_.input_.clear();

        //        //
        //        //	parse hayes/AT command
        //        //
        //        return cmd.parse(parser_, str);
        //    }
        //    parser_.input_.put(c_);
        //    return STATE_COMMAND;
        //}

        // parser::state parser::state_visitor::operator()(stream &cmd) const {
        //     if (cmd.is_esc(c_)) {
        //         return STATE_ESC;
        //     }
        //     parser_.input_.put(c_);
        //     return STATE_STREAM;
        // }

        // parser::state parser::state_visitor::operator()(esc &cmd) const {
        //     if (cmd.is_esc(c_)) {
        //         cmd.counter_++;
        //         auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
        //         cmd.start_); if (cmd.counter_ == 3) {
        //             if (delta > parser_.guard_time_) {
        //                 parser_.cb_(cyng::generate_invoke("print.ok"));
        //                 parser_.cb_(cyng::generate_invoke("stream.flush"));
        //                 parser_.cb_(cyng::generate_invoke("log.msg.debug", "switch parser in command mode"));
        //                 return STATE_COMMAND;
        //             } else {
        //                 for (std::size_t idx = 0; idx < cmd.counter_; ++idx) {
        //                     parser_.input_.put('+');
        //                 }
        //                 return STATE_STREAM;
        //             }
        //         } else {
        //             return STATE_ESC;
        //         }
        //     }

        //    for (std::size_t idx = 0; idx < cmd.counter_; ++idx) {
        //        parser_.input_.put('+');
        //    }
        //    return STATE_STREAM;
        //}

        // parser::command::command(){};

        // bool parser::command::is_eol(char c) { return (c == '\n') || (c == '\r'); }

        // parser::state parser::command::parse(parser &p, std::string inp) {
        //     //
        //     //	optimization hint: instead of boost::algorithm::erase_head() an
        //     //	iterator that would be incremented could be faster.
        //     //

        //    if (inp.empty()) {
        //        return STATE_COMMAND;
        //    } else if (boost::algorithm::starts_with(inp, "AT") || boost::algorithm::starts_with(inp, "at")) {
        //        boost::algorithm::erase_head(inp, 2);
        //        p.cb_(cyng::generate_invoke("log.msg.debug", "parsing command", inp));

        //        if (inp.empty()) {
        //            p.cb_(cyng::generate_invoke("print.ok"));
        //            p.cb_(cyng::generate_invoke("stream.flush"));
        //        } else if (boost::algorithm::starts_with(inp, "+")) {
        //            // AT+name?password
        //            boost::algorithm::erase_head(inp, 1);
        //            std::vector<std::string> result;
        //            boost::algorithm::split(result, inp, boost::is_any_of("?"), boost::token_compress_on);
        //            if (result.size() == 2) {
        //                p.cb_(cyng::generate_invoke(
        //                    "modem.req.login.public",
        //                    cyng::code::IDENT,
        //                    result.at(0),
        //                    result.at(1),
        //                    cyng::invoke("ip.tcp.socket.ep.local"),
        //                    cyng::invoke("ip.tcp.socket.ep.remote")));
        //            } else {
        //                p.cb_(cyng::generate_invoke("log.msg.error", "AT+name?password expected but got", inp));
        //                p.cb_(cyng::generate_invoke("print.error"));
        //                p.cb_(cyng::generate_invoke("stream.flush"));
        //            }
        //        } else if (boost::algorithm::starts_with(inp, "D") || boost::algorithm::starts_with(inp, "d")) {
        //            //	dial up
        //            boost::algorithm::erase_head(inp, 1);
        //            p.cb_(cyng::generate_invoke("modem.req.open.connection", cyng::code::IDENT, inp));

        //        } else if (boost::algorithm::starts_with(inp, "H") || boost::algorithm::starts_with(inp, "h")) {
        //            //	ATH - hang up
        //            boost::algorithm::erase_head(inp, 1);
        //            p.cb_(cyng::generate_invoke("modem.req.close.connection", cyng::code::IDENT));

        //        } else if (boost::algorithm::starts_with(inp, "I") || boost::algorithm::starts_with(inp, "i")) {
        //            //	ATI - informational output
        //            boost::algorithm::erase_head(inp, 1);

        //            try {
        //                const auto info = std::stoul(inp);
        //                p.cb_(cyng::generate_invoke("modem.req.info", cyng::code::IDENT, static_cast<std::uint32_t>(info)));
        //            } catch (std::exception const &ex) {
        //                boost::ignore_unused(ex);
        //                p.cb_(cyng::generate_invoke("modem.req.info", cyng::code::IDENT, static_cast<std::uint32_t>(0u)));
        //            }

        //        } else if (boost::algorithm::starts_with(inp, "O") || boost::algorithm::starts_with(inp, "o")) {
        //            //	return to stream mode
        //            return STATE_STREAM;
        //        } else {
        //            p.cb_(cyng::generate_invoke("log.msg.error", "unknown AT command", inp));
        //            p.cb_(cyng::generate_invoke("print.error"));
        //            p.cb_(cyng::generate_invoke("stream.flush"));
        //        }
        //    } else {
        //        p.cb_(cyng::generate_invoke("log.msg.error", "AT expected but got", inp));
        //        p.cb_(cyng::generate_invoke("print.error"));
        //        p.cb_(cyng::generate_invoke("stream.flush"));
        //    }
        //    return STATE_COMMAND;
        //}

        // parser::stream::stream(){};

        parser::esc::esc()
            : start_(std::chrono::system_clock::now())
            , counter_(1){};

        parser::esc::esc(esc const &other)
            : start_(other.start_)
            , counter_(other.counter_) {}

        bool parser::esc::is_complete() const { return counter_ > 2; }

        bool parser::esc::is_on_time(std::chrono::milliseconds guard) const {
            auto const delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_);
            return delta < guard;
        }

        parser::esc &parser::esc::operator++() {
            ++counter_;
            return *this;
        }

        parser::esc parser::esc::operator++(int) {
            esc temp = *this;
            ++*this;
            return temp;
        }

    } // namespace modem
} // namespace smf
