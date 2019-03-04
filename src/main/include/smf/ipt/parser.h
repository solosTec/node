/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_IPT_PARSER_H
#define NODE_IPT_PARSER_H


#include <smf/ipt/header.h>
#include <smf/ipt/scramble_key.h>

#include <cyng/intrinsics/sets.h>
#include <cyng/crypto/scrambler.hpp>
#include <cyng/vm/generator.h>

#include <boost/asio.hpp>
#include <boost/variant.hpp>
#include <functional>
#include <stack>
#include <type_traits>

namespace node 
{
	namespace ipt
	{
		/**
		 *	Whenever a IP-T command is complete a callback is triggered to process
		 *	the generated instructions. This is to guarantee to stay in the
		 *	correct state (since IP-T is statefull protocol). Especially the scrambled
		 *	login requires special consideration, because of the  provided scramble key.
		 *	In addition after a failed login no more instructions should be processed.
		 */
		class parser
		{
		public:
			using parser_callback = std::function<void(cyng::vector_t&&)>;
			using scrambler_t = cyng::crypto::scrambler<char, scramble_key::SCRAMBLE_KEY_LENGTH>;

		private:
			/**
			 * This enum stores the global state
			 * of the parser. For each state there
			 * are different helper variables mostly
			 * declared in the private section of this class.
			 */
			enum state
			{
				STATE_STREAM,
				STATE_ESC,
				STATE_HEAD,
				STATE_DATA,
			};

			struct base{
				std::size_t pos_;
				base() : pos_(0) {}	
				void reset();
			};
			struct command : base
			{
				char overlay_[HEADER_SIZE];

				command() 
					: base()
				{
					BOOST_ASSERT(pos_ == 0);
					reset();
				}

				void reset();
			};
			struct tp_req_open_push_channel : base {
				tp_req_open_push_channel() {}
			};
			struct tp_res_open_push_channel : base {
				tp_res_open_push_channel() {}
			};
			struct tp_req_close_push_channel : base {
				tp_req_close_push_channel() {}
			};
			struct tp_res_close_push_channel : base {
				tp_res_close_push_channel() {}
			};
			struct tp_req_pushdata_transfer : base {
				tp_req_pushdata_transfer() {}
			};
			struct tp_res_pushdata_transfer : base {
				tp_res_pushdata_transfer() {}
			};
			struct tp_req_open_connection : base {
				tp_req_open_connection() {}
			};
			struct tp_res_open_connection : base {
				tp_res_open_connection() {}
			};
			struct tp_res_close_connection : base {
				tp_res_close_connection() {}
			};
			//	open stream channel
			//TP_REQ_OPEN_STREAM_CHANNEL = 0x9006,
			//TP_RES_OPEN_STREAM_CHANNEL = 0x1006,

			//	close stream channel
			//TP_REQ_CLOSE_STREAM_CHANNEL = 0x9007,
			//TP_RES_CLOSE_STREAM_CHANNEL = 0x1007,

			//	stream channel data transfer
			//TP_REQ_STREAMDATA_TRANSFER = 0x9008,
			//TP_RES_STREAMDATA_TRANSFER = 0x1008,

			struct app_res_protocol_version : base {
				app_res_protocol_version() {}
			};
			struct app_res_software_version : base {
				app_res_software_version() {}
			};
			struct app_res_device_identifier : base {
				app_res_device_identifier() {}
			};
			struct app_res_network_status : base {
				app_res_network_status() {}
			};
			struct app_res_ip_statistics : base {
				app_res_ip_statistics() {}
			};
			struct app_res_device_authentification : base {
				app_res_device_authentification() {}
			};
			struct app_res_device_time : base {
				app_res_device_time() {}
			};
			struct app_res_push_target_namelist : base {
				app_res_push_target_namelist() {}
			};
			struct app_res_push_target_echo : base {
				app_res_push_target_echo() {}
			};
			struct app_res_traceroute : base {
				app_res_traceroute() {}
			};

			struct ctrl_req_login_public : base {
				ctrl_req_login_public() {}
			};
			struct ctrl_req_login_scrambled : base {
				ctrl_req_login_scrambled() {}
			};
			struct ctrl_res_login_public : base {
				ctrl_res_login_public() {}
			};
			struct ctrl_res_login_scrambled : base {
				ctrl_res_login_scrambled() {}
			};

			struct ctrl_req_logout : base {
				ctrl_req_logout() {}	//	deprecated
			};
			struct ctrl_res_logout : base {
				ctrl_res_logout() {}	//	deprecated
			};

			struct ctrl_req_register_target : base {
				ctrl_req_register_target() {}
			};
			struct ctrl_res_register_target : base {
				ctrl_res_register_target() {}
			};
			struct ctrl_req_deregister_target : base {
				ctrl_req_deregister_target() {}
			};
			struct ctrl_res_deregister_target : base {
				ctrl_res_deregister_target() {}
			};
			struct ctrl_req_watchdog : base {
				ctrl_req_watchdog() {}	//	0xC008
			};
			struct ctrl_res_watchdog : base {
				ctrl_res_watchdog() {}	//	0x4008
			};

			//	stream source register
			//CTRL_REQ_REGISTER_STREAM_SOURCE = 0xC00B,
			//CTRL_RES_REGISTER_STREAM_SOURCE = 0x400B,

			//	stream source deregister
			//CTRL_REQ_DEREGISTER_STREAM_SOURCE = 0xC00C,
			//CTRL_RES_DEREGISTER_STREAM_SOURCE = 0x400C,

			struct unknown_cmd : base {
				unknown_cmd() {}	//	0x7fff
			};


			using parser_state_t = boost::variant<command,
				tp_req_open_push_channel,	//	0x9000
				tp_res_open_push_channel,	//	0x1000
				tp_req_close_push_channel,	//	0x9001
				tp_res_close_push_channel,	//	0x1001
				tp_req_pushdata_transfer,	//	0x9002
				tp_res_pushdata_transfer,	//	0x1002
				tp_req_open_connection,		//	0x9003
				tp_res_open_connection,		//	0x1003
				//tp_req_close_connection,	//	0x9004
				tp_res_close_connection,	//	0x1004

				//app_req_protocol_version,	//	0xa000
				app_res_protocol_version,	//	0x2000
				app_res_software_version,	//	0x2001
											//	0x2002
				app_res_device_identifier,	//	0x2003
				app_res_network_status,		//	0x2004
				app_res_ip_statistics,		//	0x2005
				app_res_device_authentification,		//	0x2006
				app_res_device_time,		//	0x2007
				app_res_push_target_namelist,		//	0x2008
				app_res_push_target_echo,		//	0x2009
				app_res_traceroute,			//	0x200a

				ctrl_req_login_public,		//	0xc001
				ctrl_req_login_scrambled,	//	0xc002
				ctrl_res_login_public,		//	0x4001
				ctrl_res_login_scrambled,	//	0x4002

				ctrl_req_logout,			//	0xc004
				ctrl_res_logout,			//	0x4004

				ctrl_req_register_target,	//	0xc005
				ctrl_res_register_target,	//	0x4005
				ctrl_req_deregister_target,	//	0xc006
				ctrl_res_deregister_target,	//	0x4006
				//ctrl_req_watchdog,			//	0xc008
				//ctrl_res_watchdog			//	0x4008

				unknown_cmd					//	0x7fff
			>;

			//
			//	signals
			//
			struct state_visitor : public boost::static_visitor<state>
			{
				state_visitor(parser&, char c);
				state operator()(command& cmd) const;
				state operator()(tp_req_open_push_channel&) const;
				state operator()(tp_res_open_push_channel&) const;
				state operator()(tp_req_close_push_channel&) const;
				state operator()(tp_res_close_push_channel&) const;
				state operator()(tp_req_pushdata_transfer&) const;
				state operator()(tp_res_pushdata_transfer&) const;
				state operator()(tp_req_open_connection&) const;
				state operator()(tp_res_open_connection&) const;
				//state operator()(tp_req_close_connection&) const;
				state operator()(tp_res_close_connection&) const;

				state operator()(app_res_protocol_version&) const;
				state operator()(app_res_software_version&) const;
				state operator()(app_res_device_identifier&) const;
				state operator()(app_res_network_status&) const;
				state operator()(app_res_ip_statistics&) const;
				state operator()(app_res_device_authentification&) const;
				state operator()(app_res_device_time&) const;
				state operator()(app_res_push_target_namelist&) const;
				state operator()(app_res_push_target_echo&) const;
				state operator()(app_res_traceroute&) const;


				state operator()(ctrl_req_login_public&) const;
				state operator()(ctrl_req_login_scrambled&) const;
				state operator()(ctrl_res_login_public&) const;
				state operator()(ctrl_res_login_scrambled&) const;

				state operator()(ctrl_req_logout&) const;
				state operator()(ctrl_res_logout&) const;

				state operator()(ctrl_req_register_target&) const;
				state operator()(ctrl_res_register_target&) const;
				state operator()(ctrl_req_deregister_target&) const;
				state operator()(ctrl_res_deregister_target&) const;

				state operator()(unknown_cmd&) const;

				parser& parser_;
				const char c_;
			};

		public:
			/**
			 * @param cb this function is called, when parsing is complete
			 */
			parser(parser_callback cb, scramble_key const&);

			/**
			 * The destructor is required since the unique_ptr
			 * uses an incomplete type.
			 */
			virtual ~parser();

			/**
			 * parse the specified range
			 */
			template < typename I >
			cyng::buffer_t read(I start, I end)
			{
				cyng::buffer_t buffer;
				buffer.reserve(std::distance(start, end));
				std::for_each(start, end, [this, &buffer](char c)
				{
					//
					//	Decode input stream
					//
					buffer.push_back(this->put(scrambler_[c]));
				});

				if (read_counter_ == 0u && buffer.size() > 1) {
					//BOOST_ASSERT_MSG((buffer.at(0) == 0x01 || buffer.at(0) == 0x02), "IP-T login expected (0)");
					//BOOST_ASSERT_MSG((buffer.at(1) == (char)0x40 || buffer.at(1) == (char)0xc0), "IP-T login expected (1)");
					if (!((buffer.at(0) == 0x01 || buffer.at(0) == 0x02) && (buffer.at(1) == (char)0x40 || buffer.at(1) == (char)0xc0))) {
						cb_(std::move(cyng::generate_invoke("log.msg.fatal", "IP-T login expected", buffer)));
					}
				}

				post_processing();
				return buffer;
			}

			/**
			 * Client has to set new scrambled after login request
			 */
			void set_sk(scramble_key const&);

			/**
			 * Reset parser (default scramble key)
			 */
			void reset(scramble_key const&);

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

			/**
			 * Read a single string from input stream.
			 * This is safe since each ipt requires to terminate
			 * each string with 0x00.
			 */
			std::string read_string();

			/**
			 * Read a scramble key
			 */
			scramble_key read_sk();

			/**
			 * read data block
			 */
			cyng::buffer_t read_data();

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
			std::function<std::string()> get_read_string_f();
			std::function<std::uint8_t()> get_read_uint8_f();
			std::function<std::uint16_t()> get_read_uint16_f();
			std::function<std::uint32_t()> get_read_uint32_f();
			std::function<std::uint64_t()> get_read_uint64_f();
			std::function<cyng::buffer_t()> get_read_data_f();

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

			scramble_key def_sk_;	//!<	system wide default scramble key

			/**
			 * Decrypting data stream
			 */
			scrambler_t	scrambler_;

			state	stream_state_;
			parser_state_t	parser_state_;

#ifdef _DEBUG
			/**
			 * Additional parser state test: The first command has to be a login request/response.
			 * After that no further logins are allowed.
			 */
			bool authorized_;
#endif
			/**
			 * mainly used to detect the first call of the parser
			 */
			std::size_t read_counter_;

			//
			//	current header
			//
			header	header_;

		};
	}
}	//	node

#endif // NODE_IPT_PARSER_H
