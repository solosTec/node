/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_MODEM_PARSER_H
#define NODE_MODEM_PARSER_H

#include <cyng/intrinsics/sets.h>
#include <cyng/intrinsics/buffer.h>

#include <boost/asio.hpp>
#include <boost/variant.hpp>
#include <functional>
#include <stack>
#include <type_traits>

namespace node 
{
	namespace modem
	{
		/**
		 *	Whenever a modem/AT command is complete a callback is triggered to process
		 *	the generated instructions. This is to guarantee to stay in the
		 *	correct state. 
		 *
		 *	Defines the modem protocol state machine.
		 * The modem state machine has three states: The command, data and
		 * escape state. The machine starts in the command mode. After receiving
		 * the "ATA" command ist switches into the data mode. An escape symbol
		 * (+++) followed by an "ATH" switches back into command mode.
		 * @code

		      +
		      |
		      |
		      |
	          v
		 +----+----- ----------------------------------------------------+
		 |          -                                                    |
		 |   start  |                                                    |
		 |          v                                                    |
		 |  +-------+---+  +--- --------------------------------------+  |
		 |  |           |  |   -                                      |  |
		 |  | command   |  |   |       stream                         |  |
		 |  |           |  |   v                                      |  |
		 |  |           |  | +-+----------------+ +-----------------+ |  |
		 |  |           |  | |                  | |                 | |  |
		 |  |           |  | | data             | | escape          | |  |
		 |  |           |  | |                  | |                 | |  |
		 |  |           |  | |                  | |                 | |  |
		 |  |           |  | |                  | |                 | |  |
		 |  |           |  | |                  | |                 | |  |
		 |  |           |  | |                  | |                 | |  |
		 |  |           |  | |                  | |                 | |  |
		 |  |           |  | +------------------+ +-----------------+ |  |
		 |  |           |  |                                          |  |
		 |  +-----------+  +------------------------------------------+  |
		 |                                                               |
		 +---------------------------------------------------------------+
		 * @endcode
		 */

		class parser
		{
		public:
			using parser_callback = std::function<void(cyng::vector_t&&)>;

		private:
			/**
			 * This enum stores the global state
			 * of the parser. For each state there
			 * are different helper variables mostly
			 * declared in the private section of this class.
			 */
			enum state
			{
				STATE_COMMAND,
				STATE_STREAM,
				STATE_ESC,
				STATE_ERROR,
			};

			struct command
			{
				command();

				static bool is_eol(char);
				static state parse(parser&, std::string);

			};
			struct stream
			{
				stream();
				static bool is_esc(char);
			};

			struct esc
			{
				esc();
				esc(esc const&);
				static bool is_esc(char);

				std::chrono::system_clock::time_point start_;
				std::size_t counter_;
			};

			using parser_state_t = boost::variant<
				command,
				stream,
				esc
			>;

			//
			//	signals
			//
			struct state_visitor : public boost::static_visitor<state>
			{
				state_visitor(parser&, char c);
				state operator()(command& cmd) const;
				state operator()(stream&) const;
				state operator()(esc&) const;

				parser& parser_;
				const char c_;
			};

		public:
			/**
			 * @param cb this function is called, when parsing is complete
			 */
			parser(parser_callback cb, std::chrono::milliseconds guard_time);

			/**
			 * The destructor is required since the unique_ptr
			 * uses an incomplete type.
			 */
			virtual ~parser();

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
					this->put(c);
				});

				post_processing();
				return std::distance(start, end);
			}

			/**
			 * transit to stream mode
			 */
			void set_stream_mode();

			/**
			 * transit to command mode
			 */
			void set_cmd_mode();

			/**
			 * Clear internal state and all callback function
			 */
			void clear();

		private:
			/**
			 * read a single byte and update
			 * parser state.
			 * Implements the state machine
			 */
			char put(char);

			/**
			 * Probe if parsing is completed and
			 * inform listener.
			 */
			void post_processing();

		private:
			/**
			 * call this method if parsing is complete
			 */
			parser_callback	cb_;

			const std::chrono::milliseconds guard_time_;

			/**
			 * instruction buffer
			 */
			cyng::vector_t	code_;

			state	stream_state_;
			parser_state_t	parser_state_;

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
}	//	node

#endif // NODE_MODEM_PARSER_H
