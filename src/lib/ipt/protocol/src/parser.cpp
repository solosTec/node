/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/ipt/parser.h>
#include <smf/ipt/codes.h>

namespace smf
{
	namespace ipt	
	{
		parser::parser(scramble_key const& sk, command_cb command_complete, data_cb transmit)
		: state_(state::HEADER)
			, def_sk_(sk)
			, command_complete_(command_complete)
			, transmit_(transmit)
			, scrambler_()
			, buffer_()
			, header_()
		{
			buffer_.reserve(HEADER_SIZE);
			header_.reset(INVALID_SEQ);
		}

		char parser::put(char c) {

			switch (state_) {
			case state::STREAM:
				state_ = state_stream(c);
				break;
			case state::ESC:
				state_ = state_esc(c);
				break;
			case state::HEADER:
				state_ = state_header(c);
				break;
			case state::DATA:
				state_ = state_data(c);
				break;
			default:
				BOOST_ASSERT_MSG(false, "invalid parser state");
				break;
			}

			return c;
		}

		parser::state parser::state_stream(char c) {
			if (c == ESCAPE_SIGN)	return state::ESC;
			buffer_.push_back(c);
			return state_;
		}
		parser::state parser::state_esc(char c) {
			if (c == ESCAPE_SIGN) {
				buffer_.push_back(c);
				return state::STREAM;
			}
			return state::HEADER;
		}
		parser::state parser::state_header(char c) {
			buffer_.push_back(c);
			if (buffer_.size() == HEADER_SIZE) {
				header_ = to_header(buffer_);
				buffer_.clear();

				if (has_data(header_)) {

					//
					//	collect data
					//
					buffer_.reserve(size(header_));
					return state::DATA;

				}
				else {

					//
					//	command complete
					//
					command_complete_(header_, std::move(buffer_));

					buffer_.reserve(HEADER_SIZE);
					return state::HEADER;
				}
			}
			return state_;
		}

		parser::state parser::state_data(char c) {
			buffer_.push_back(c);
			if (buffer_.size() == size(header_)) {

				//
				//	command complete
				//
				command_complete_(header_, std::move(buffer_));
				buffer_.clear();
				buffer_.reserve(HEADER_SIZE);
				return state::HEADER;
			}
			return state_;
		}


		/**
		 * Probe if parsing is completed and
		 * inform listener.
		 */
		void parser::post_processing() {
			if (state_ == state::STREAM && !buffer_.empty()) {

				//
				//	transmit data
				//
				transmit_(std::move(buffer_));
			}
		}

		void parser::set_sk(scramble_key const& sk)
		{
			scrambler_ = sk.key();
		}

		void parser::reset(scramble_key const& sk)
		{
			clear();
			def_sk_ = sk.key();
		}

		void parser::clear()
		{
			state_ = state::HEADER;
			header_.reset(INVALID_SEQ);
			scrambler_.reset();
			buffer_.clear();
		}

	}	//	ipt	
}	
