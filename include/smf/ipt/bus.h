/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_BUS_H
#define SMF_IPT_BUS_H

#include <smf/ipt/config.h>
#include <smf/ipt/serializer.h>
#include <smf/ipt/parser.h>

#include <cyng/log/logger.h>

namespace smf
{
	namespace ipt	
	{
		class bus
		{
			enum class state {
				START,
				CONNECTED,
				AUTHORIZED,
				LINKED,
				STOPPED,
			} state_;

			/**
			 * Use this to store a (target) name - channel relation
			 */
			using name_channel_t = std::pair<std::string, cyng::channel_weak>;

		public:
			//	call back to signal changed IP-T authorization state
			using auth_cb = std::function<void(bool)>;

		public:
			bus(boost::asio::io_context& ctx
				, cyng::logger
				, toggle::server_vec_t&&
				, std::string model
				, parser::command_cb
				, parser::data_cb
				, auth_cb);

			void start();
			void stop();
			bool is_authorized() const;

			void register_target(std::string, cyng::channel_weak);

		private:
			constexpr bool is_stopped() const {
				return state_ == state::STOPPED;
			}
			void reset();
			void connect(boost::asio::ip::tcp::resolver::results_type endpoints);
			void start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
			void handle_connect(boost::system::error_code const& ec,
				boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
			void check_deadline(boost::system::error_code const&);
			void do_read();
			void do_write();
			void handle_read(boost::system::error_code const& ec, std::size_t n);
			void handle_write(boost::system::error_code const& ec);

			/**
			 * start an async write
			 */
			void send(cyng::buffer_t);

			void cmd_complete(header const&, cyng::buffer_t&&);

			void res_login(cyng::buffer_t&&);
			void res_register_target(header const&, cyng::buffer_t&&);

			void pushdata_transfer(header const&, cyng::buffer_t&&);

			void req_watchdog(header const&, cyng::buffer_t&&);

			void open_connection(std::string, sequence_t);
			void close_connection(sequence_t);


		private:
			cyng::logger logger_;
			toggle tgl_;
			std::string const model_;
			parser::command_cb cb_cmd_;
			auth_cb cb_auth_;

			boost::asio::ip::tcp::resolver::results_type endpoints_;
			boost::asio::ip::tcp::socket socket_;
			boost::asio::steady_timer timer_;
			boost::asio::io_context::strand dispatcher_;
			serializer	serializer_;
			parser	parser_;
			std::deque<cyng::buffer_t>	buffer_write_;
			std::array<char, 2048>	input_buffer_;

			/**
			 * Temporary table to hold a "register target" request.
			 * After receiving a response the stored callback will be moved
			 * to the channel => callback map.
			 */
			std::map< ipt::sequence_t, name_channel_t >	registrant_;
			std::map< std::uint32_t, name_channel_t >	targets_;

		};
	}	//	ipt
}


#endif
