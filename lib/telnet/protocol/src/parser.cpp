/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include <smf/telnet/parser.h>
#include <smf/telnet/defs.h>

#include <cyng/intrinsics/buffer.h>

namespace node
{
	namespace telnet
	{
		parser::parser()
			: state_(STATE_DATA)
			, stream_buffer_()
			, input_(&stream_buffer_)
		{}

		void parser::post_processing()
		{
			if (state_ == STATE_DATA)
			{
				//
				//	use 
				//	std::istreambuf_iterator<char> eos; 
				//	if {} doesn't work
				cyng::buffer_t buffer(std::istreambuf_iterator<char>(input_), {});
				if (!buffer.empty())
				{
					//cb_(cyng::generate_invoke("telnet.req.transmit.data", cyng::code::IDENT, buffer));
				}
			}
		}

		parser::state parser::put(char c)
		{
			switch (static_cast<std::uint8_t>(state_)) {
			case STATE_DATA:
				switch (c) {
				case TELNET_IAC:
					return STATE_IAC;
				case '\r':	//	CR
					return STATE_EOL;
				default:
					input_.put(c);
					break;
				}
				break;

			case STATE_EOL:
				return STATE_DATA;

			case STATE_IAC:
				switch (c) {
				case TELNET_SB:	return STATE_SB;
				case TELNET_WILL: return STATE_WILL;
				case TELNET_WONT: return STATE_WONT;
				case TELNET_DO:	return STATE_DO;
				case TELNET_DONT: return STATE_DONT;
				case TELNET_IAC: return STATE_DATA;
				default:
					break;
				}
				return STATE_DATA;

			case STATE_WILL:
			case STATE_WONT:
			case STATE_DO:
			case STATE_DONT:
				return STATE_DATA;

			case STATE_SB:
				return STATE_DATA;

			case STATE_SB_DATA:
				switch (c) {
				case TELNET_IAC: return STATE_SB_DATA_IAC;
				default:
					break;
				}
				return STATE_DATA;

			case STATE_SB_DATA_IAC:
				switch (c) {
				case TELNET_SE: return STATE_DATA;
				case TELNET_IAC: 
					//return STATE_SB_DATA;
					return STATE_DATA;
				default:
					break;
				}
				return STATE_DATA;

			default:
				break;
			}
			return state_;
		}
	}
}
