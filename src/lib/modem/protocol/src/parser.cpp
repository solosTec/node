/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */
#include <smf/modem/parser.h>
// #include <cyng/vm/generator.h>

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

        bool parser::put(char c) {
            bool advance{true};
            switch (state_) {
            case state::COMMAND: std::tie(state_, advance) = state_command(c); break;
            case state::DATA: std::tie(state_, advance) = state_data(c); break;
            case state::STREAM: std::tie(state_, advance) = state_stream(c); break;
            case state::ESC: std::tie(state_, advance) = state_esc(c); break;
            default: break;
            }

            return advance;
        }

        std::string parser::get_mode() const {
            switch (state_) {
            case state::COMMAND: return "COMMAND";
            case state::DATA: return "DATA";
            case state::STREAM: return "STREAM";
            case state::ESC: return "ESC";
            default: break;
            }
            return "ERROR";
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
                            buffer_.clear();
                        }
                        command_cb_("+++", "");
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
                buffer_.clear();
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

        parser::esc::esc()
            : start_(std::chrono::system_clock::now())
            , counter_(1){};

        parser::esc::esc(esc const &other)
            : start_(other.start_)
            , counter_(other.counter_) {}

        bool parser::esc::is_complete() const { return counter_ > 2; }

        bool parser::esc::is_on_time(std::chrono::milliseconds guard) const {
            auto const delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_);
            return delta >= guard;
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
