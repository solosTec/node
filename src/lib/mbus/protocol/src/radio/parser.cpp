/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/mbus/radio/parser.h>

#include <boost/assert.hpp>

#include <iostream>

namespace smf
{
	namespace mbus
	{
		namespace radio
		{
			parser::parser(cb_t cb)
				: state_(state::HEADER)
				, cb_(cb)
				, pos_{ 0 }
				, header_{}
				, payload_()
			{}

			void parser::put(char c) {

				switch (state_) {
				case state::HEADER:
					state_ = state_header(c);
					break;
				case state::DATA:
					state_ = state_data(c);
					break;
				default:
					BOOST_ASSERT_MSG(false, "illegal parser state");
					break;
				}
			}

			parser::state parser::state_header(char c) {
				BOOST_ASSERT(pos_ < header::size());
				header_.data_[pos_++] = c;
				if (pos_ == header::size()) {
					pos_ = 0;
					payload_.clear();
					payload_.push_back(c);	//	application type (CI)
					if (header_.payload_size() < 3) {
						cb_(header_, payload_);
						return state::HEADER;
					}
					payload_.reserve(header_.payload_size());
					return state::DATA;
				}
				return state_;
			}

			parser::state parser::state_data(char c) {
				payload_.push_back(c);
				BOOST_ASSERT(payload_.back() == c);
				if (payload_.size() == header_.payload_size()) {
					BOOST_ASSERT(!payload_.empty());
#ifdef _DEBUG
					auto const fth = header_.get_frame_type();
					auto const ftp = payload_.at(0);
					BOOST_ASSERT(fth == ftp);
#endif
					cb_(header_, payload_);
					return state::HEADER;
				}
				return state_;
			}

		}
	}
}


