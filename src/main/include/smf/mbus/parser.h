/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_MBUS_PARSER_H
#define NODE_MBUS_PARSER_H


#include <smf/mbus/defs.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/intrinsics/buffer.h>

#include <boost/asio.hpp>
#include <boost/variant.hpp>
#include <functional>
#include <stack>
#include <type_traits>

namespace node 
{
	namespace mbus
	{
		/**
		 *
		 * Packet formats:
		 *
		 * ACK: size = 1 byte
		 *
		 *   byte1: ack   = 0xE5
		 *
		 * SHORT: size = 5 byte
		 *
		 *   byte1: start   = 0x10
		 *   byte2: control = ...
		 *   byte3: address = ...
		 *   byte4: chksum  = ...
		 *   byte5: stop    = 0x16
		 *
		 * CONTROL: size = 9 byte
		 *
		 *   byte1: start1  = 0x68
		 *   byte2: length1 = ...
		 *   byte3: length2 = ...
		 *   byte4: start2  = 0x68
		 *   byte5: control = ...
		 *   byte6: address = ...
		 *   byte7: ctl.info= ...
		 *   byte8: chksum  = ...
		 *   byte9: stop    = 0x16
		 *
		 * LONG: size = N >= 9 byte
		 *
		 *   byte1: start1  = 0x68
		 *   byte2: length1 = ...
		 *   byte3: length2 = ...
		 *   byte4: start2  = 0x68
		 *   byte5: control = ...
		 *   byte6: address = ...
		 *   byte7: ctl.info= ...
		 *   byte8: data    = ...
		 *          ...     = ...
		 *   byteN-1: chksum  = ...
		 *   byteN: stop    = 0x16
		 *
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
				STATE_START,
				STATE_FRAME_SHORT,
				STATE_FRAME,
				//STATE_CONTROL_START,
				//STATE_LONG_START,
				//STATE_STOP,
			};

			struct base {
				std::size_t pos_;
				std::uint8_t checksum_;
				bool ok_;	//	checksum ok
				base() 
					: pos_(0) 
					, checksum_(0)
					, ok_(false)
				{}
			};
			//struct frame {

			//	std::uint8_t start1;
			//	std::uint8_t length1;
			//	std::uint8_t length2;
			//	std::uint8_t start2;
			//	std::uint8_t control;
			//	std::uint8_t address;
			//	std::uint8_t control_information;
			//	// variable data field
			//	std::uint8_t checksum;
			//	std::uint8_t stop;

			//	std::uint8_t   data[FRAME_DATA_LENGTH];
			//	size_t data_size;

			//	int type;
			//	time_t timestamp;

			//};

			//struct command : base
			//{
			//	char overlay_[FRAME_DATA_LENGTH];

			//	command() {}
			//};

			struct ack : base {
				ack() : base() {}
			};
			struct short_frame : base {
				short_frame() : base() {}
			};
			struct long_frame : base {
				long_frame() : base() {}
			};
			struct ctrl_frame : base {
				ctrl_frame() : base() {}
			};


			using parser_state_t = boost::variant<ack,
				short_frame,	//	0x10
				long_frame,	//	0x68
				ctrl_frame	//	0x68
			>;

			//
			//	signals
			//
			struct state_visitor : boost::static_visitor<state>
			{
				state_visitor(parser&, char c);
				state operator()(ack&) const;
				state operator()(short_frame&) const;
				state operator()(long_frame&) const;
				state operator()(ctrl_frame&) const;

				parser& parser_;
				const char c_;
			};

		public:
			/**
			 * @param cb this function is called, when parsing is complete
			 */
			parser(parser_callback cb);

			/**
			 * The destructor is required since the unique_ptr
			 * uses an incomplete type.
			 */
			virtual ~parser();

			/**
			 * parse the specified range
			 */
			template < typename I >
			void read(I start, I end)
			{
				std::for_each(start, end, [this](char c)
				{
					//
					//	parse
					//
					this->put(c);
				});

				post_processing();
			}

			/**
			 * Reset parser (default scramble key)
			 */
			void reset();

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

			/**
			 * Read a numeric value
			 */
			template <typename T>
			T read_numeric()
			{
				static_assert(std::is_arithmetic<T>::value, "arithmetic type required");
				T v{ 0 };
				input_.read(reinterpret_cast<std::istream::char_type*>(&v), sizeof(v));
				return v;
			}

		private:
			/**
			 * call this method if parsing is complete
			 */
			parser_callback	cb_;

			/**
			 * instruction buffer
			 */
			cyng::vector_t	code_;

			/**
			 *	input stream buffer
			 */
			boost::asio::streambuf	stream_buffer_;

			/**
			 *	input stream
			 */
			std::iostream			input_;


			state	stream_state_;
			parser_state_t	parser_state_;

			//
			//	to prevent statement ordering we have to use
			//	function objects instead of function results
			//	in the argument list
			//
			//std::function<std::string()> f_read_string;
			std::function<std::uint8_t()> f_read_uint8;
			//std::function<std::uint16_t()> f_read_uint16;
			//std::function<std::uint32_t()> f_read_uint32;
			//std::function<std::uint64_t()> f_read_uint64;
			//std::function<cyng::buffer_t()> f_read_data;

		};
	}
}	//	node

#endif // NODE_MBUS_PARSER_H
