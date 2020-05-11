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
#include <array>
#ifdef _DEBUG
#include <set>
#endif

namespace node 
{
	namespace mbus
	{
		/**
		 * Parser for (wired/wireless) m-bus communication.
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
			enum class state
			{
				INVALID_STATE,
				START,
				FRAME_SHORT,
				FRAME,
				FRAME_CTRL,
				FRAME_LONG
			};

			struct base {
				std::uint8_t pos_;
				std::uint8_t cs_;	//	checksum
				bool ok_;	//	checksum ok
				base()
					: pos_(0)
					, cs_(0)
					, ok_(false)
				{}
			};

			struct ack : base {
				ack() : base() {}
			};
			struct short_frame : base {
				std::uint8_t c_, a_;
				short_frame() 
					: base() 
					, c_(0)
					, a_(0)
				{}
			};
			struct long_frame : base {
				std::uint8_t c_, a_, ci_;
				cyng::buffer_t data_;
				long_frame() 
					: base() 
					, c_(0)
					, a_(0)
					, ci_(0)
					, data_()
				{}
			};
			struct ctrl_frame : base {
				std::uint8_t c_, a_, ci_;
				ctrl_frame()
					: base() 
					, c_(0)
					, a_(0)
					, ci_(0)
				{}
			};
			struct test_frame  {
				test_frame() 
					: lfield_()
				{}
				cyng::buffer_t lfield_;
			};


			using parser_state_t = boost::variant<ack,
				short_frame,	//	0x10
				long_frame,	//	0x68
				ctrl_frame,	//	0x68
				test_frame	//	check frame type (control/long)
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
				state operator()(test_frame&) const;

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

			//
			//	to prevent statement ordering we have to use
			//	function objects instead of function results
			//	in the argument list
			//
			std::function<std::uint8_t()> get_read_uint8_f();

		private:
			/**
			 * call this method if parsing is complete
			 */
			parser_callback	cb_;

			/**
			 * instruction buffer
			 */
			//cyng::vector_t	code_;

			/**
			 *	input stream buffer
			 */
			boost::asio::streambuf	stream_buffer_;

			/**
			 *	input stream
			 */
			std::iostream			input_;

			/**
			 * global parser state
			 */
			state	stream_state_;
			parser_state_t	parser_state_;

			/**
			 * checksum
			 */
			std::uint8_t checksum_;

			/**
			 * length - result from last length field
			 */
			std::uint8_t length_;

		};

	}

	namespace wmbus
	{
		/**
 		 * Parser for wireless m-bus communication.
		 *
		 * Packet formats:
		 *
		 * byte1: length (without this byte and CRC)
		 * byte2: control field
		 * 
		 * When in S1 mode:
		 *
		 * byte3-4: manufacturer ID
		 * byte5-10: address 
		 *	-> 4 bytes: device ID
		 *	-> 1 byte: version
		 *	-> 1 byte: device type
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
			enum class state
			{
				INVALID_STATE,
				LENGTH,	//	first byte contains length
				CTRL_FIELD,
				MANUFACTURER,
				DEV_ID,		//	address
				DEV_VERSION,
				DEV_TYPE,
				//STATE_CRC,
				FRAME_TYPE,
				//	0x70: report error
				//	0x72: long data header
				//	0x7A: short data header
				//	0x78: no data header
				APL_ERROR,
				HEADER_SHORT,
				HEADER_LONG,
				HEADER_NONE,
				CRC,
			};

			struct error {
			};

			template <std::size_t N>
			struct base {
				std::size_t pos_;
				std::array<char, N> data_;
				base()
					: pos_(0)
					, data_{0}
				{}

			};
			struct manufacturer : base<2> {};
			struct dev_id : base<4> {};
			//struct crc : base<2> {};

			/**
			 * Transport Layer (TPL)
			 */
			struct frame_short {
				/**
				 * The Access Number together with the transmitter address is used to identify a datagram.
				 */
				std::uint8_t access_nr_;

				/**
				 * It will be distinguished between:
				 * > Gateway Status (applied with CI-Field 5Ah, 5Bh, 60h, 61h, 64h, 65h, 6Ch, 6Dh or 80h)
				 * > Meter Status (applied with CI-Field 6Eh, 6Fh, 72h, 74h, 75h, 7Ah, 7Ch, 7Dh, 7Eh, 7Fh, 8Ah or 8Bh)
				 */
				std::uint8_t status_;

				std::uint16_t cfg_;
				std::uint8_t aes_[2];
			};

			struct frame_data {
				std::size_t size_;
				cyng::buffer_t data_;
				frame_data(std::size_t size);
				void decode_main_vif(std::uint8_t);
				void decode_E0(std::uint8_t);
				void decode_E1(std::uint8_t);
				void decode_E10(std::uint8_t);
				void decode_E11(std::uint8_t);
				void decode_E00(std::uint8_t);
				void decode_E110(std::uint8_t);
				void decode_E111(std::uint8_t);
				void decode_01(std::uint8_t);
			};

			using parser_state_t = boost::variant<error
				, manufacturer
				, dev_id
				//, crc
				, frame_data>;

			//
			//	signals
			//
			struct state_visitor : boost::static_visitor<state>
			{
				state_visitor(parser&, char c);
				state operator()(error&) const;
				state operator()(manufacturer&) const;
				//state operator()(dev_version&) const;
				state operator()(dev_id&) const;
				//state operator()(crc&) const;
				state operator()(frame_data&) const;

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
			 * Reset parser
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
			 * handle CTRL field state
			 */
			std::pair<state, parser_state_t> ctrl_field(char);
			std::pair<state, parser_state_t> frame_type(char);

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

			/**
			 * stream state
			 */
			state	stream_state_;
			parser_state_t	parser_state_;

			/**
			 * packet size without CRC (S, T, R2 Mode)
			 * packet size with CRC (C Mode)
			 */
			std::size_t	packet_size_;

			std::string manufacturer_;
			std::uint8_t version_;
			std::uint8_t medium_;
			std::uint32_t dev_id_;
			std::uint8_t frame_type_;
			/**
			 * [0] 1 byte 01/02 01 == wireless, 02 == wired
			 * [1-2] 2 bytes manufacturer ID
			 * [3-6] 4 bytes serial number
			 * [7] 1 byte device type / media
			 * [8] 1 byte product revision
			 *
			 * 9 bytes in total
			 * example: 01-e61e-13090016-3c-07
			 */
			std::array<char, 9>	server_id_;

#ifdef _DEBUG
			std::set<std::uint32_t>	meter_set_;
#endif
		};
	}
}	//	node

#endif // NODE_MBUS_PARSER_H
