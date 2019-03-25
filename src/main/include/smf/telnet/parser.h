/*
 * The MIT License (MIT)
 *
 * Copyright (c) 20189 Sylko Olzscher
 *
 */
#ifndef NODE_TELNET_PARSER_H
#define NODE_TELNET_PARSER_H

#include <iterator>
#include <boost/asio.hpp>

namespace node
{
	namespace telnet
	{
		class parser
		{
		public:
			parser();

			/**
			 * parse the specified range
			 */
			template < typename I >
			auto read(I start, I end) -> typename std::iterator_traits<I>::difference_type
			{
				std::for_each(start, end, [this](char c)
				{
					//
					//	Decode input stream
					//
					state_ = this->put(c);
				});

				post_processing();
				return std::distance(start, end);
			}

		private:
			enum state
			{
				STATE_DATA = 0,
				STATE_EOL,
				STATE_IAC,
				STATE_WILL,
				STATE_WONT,
				STATE_DO,
				STATE_DONT,
				STATE_SB,
				STATE_SB_DATA,
				STATE_SB_DATA_IAC
			} state_;

			state put(char c);
			void post_processing();

		private:
			/**
			 *	input stream buffer
			 */
			boost::asio::streambuf	stream_buffer_;

			/**
			 * input stream.
			 * Stream are not moveable (http://cplusplus.github.io/LWG/lwg-defects.html#911). So it's
			 * appropriate to hold the input stream as a member of the global parser object instead of
			 * the object that represent the current state (what would be more natural).
			 */
			std::iostream			input_;

		};
	}
}

#endif

