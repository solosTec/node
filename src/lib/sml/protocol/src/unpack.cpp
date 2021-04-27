#include <smf/sml/unpack.h>

#ifdef _DEBUG_SML
#include <iostream>
#endif

#include <boost/assert.hpp>

namespace smf {
	namespace sml {

		unpack::unpack(parser::cb f)
			: state_(state::PAYLOAD)
			, parser_(f)
			, counter_{ 0 }
			, pads_{ 0 }
			, crc_{ 0 }
		{}

		parser const& unpack::get_parser() const {
			return parser_;
		}

		void unpack::put(char c) {

			switch (state_) {
			case state::PAYLOAD:
				state_ = state_payload(c);
				break;
			case state::ESC:
				state_ = state_esc(c);
				break;
			case state::PROP:
				state_ = state_prop(c);
				break;
			case state::START:
				state_ = state_start(c);
				break;
			case state::STOP:
				state_ = state_stop(c);
				break;
			default:
				break;
			}
		}

		unpack::state unpack::state_esc(char c) {
			if (ESCAPE_SIGN == c) {
				++counter_;
				if (counter_ == 4) {
					counter_ = 0;
					return state::PROP;
				}
				return state_;
			}

			//
			//	redeliver
			//
			while (counter_-- > 0) {
				parser_.tokenizer_.put(27);
			}
			parser_.tokenizer_.put(c);
			return state::PAYLOAD;
		}

		unpack::state unpack::state_prop(char c) {

			BOOST_ASSERT(counter_ == 0);

			//	only version 1 is supported
			switch (c) {
			case 0x01:
				counter_ = 1;
				return state::START;
			case 0x02:
			case 0x03:
			case 0x04:
				BOOST_ASSERT_MSG(false, "not supported");
				break;
			case 0x1a:
				counter_ = 1;
				return state::STOP;
			default:
				BOOST_ASSERT_MSG(false, "unknown property");
				break;
			}

			//	error
			return state_payload(c);
		}

		unpack::state unpack::state_payload(char c) {
			if (ESCAPE_SIGN == c) {
				counter_ = 1;
				return state::ESC;
			}
			parser_.tokenizer_.put(c);
			return state_;
		}

		unpack::state unpack::state_start(char c) {
			if (0x01 == c) {
				++counter_;
				if (counter_ == 4) {
					counter_ = 0;
					return state::PAYLOAD;
				}
				return state_;
			}

			//
			//	redeliver
			//
			while (counter_-- > 0) {
				parser_.tokenizer_.put(1);
			}
			parser_.tokenizer_.put(c);
			return state_;
		}

		unpack::state unpack::state_stop(char c) {
			switch (counter_) {
			case 1:
				pads_ = c;
				break;
			case 2:
				crc_ = static_cast<uint16_t>(c) & 0xff;
				crc_ <<= 8;
				break;
			case 3:
				crc_ |= (static_cast<uint16_t>(c) & 0xff);
				break;
			default:
				break;
			}
			++counter_;
			if (counter_ == 4) {
				counter_ = 0;
				return state::PAYLOAD;
			}
			return state_;
		}

	}
}
