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
				INITIAL,
				CONNECTED,
				AUTHORIZED,
				LINKED
			} state_;

		public:
			bus(boost::asio::io_context& ctx
				, cyng::logger
				, toggle::server_vec_t&&);
			void start();
			void stop();

		private:
			void connect(boost::asio::ip::tcp::resolver::results_type endpoints);
			void start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
			void handle_connect(const boost::system::error_code& ec,
				boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
			void check_deadline();
			void do_read();
			void do_write();
			void handle_read(const boost::system::error_code& ec, std::size_t n);
			void handle_write(const boost::system::error_code& ec);

			void cmd_complete(header const&, cyng::buffer_t&&);
			void transmit(cyng::buffer_t&&);

			void res_login(cyng::buffer_t&&);

		private:
			cyng::logger logger_;
			toggle tgl_;
			bool stopped_;
			boost::asio::ip::tcp::resolver::results_type endpoints_;
			boost::asio::ip::tcp::socket socket_;
			boost::asio::steady_timer deadline_;
			serializer	serializer_;
			parser	parser_;
			std::deque<cyng::buffer_t>	buffer_write_;
			std::array<char, 2048>	input_buffer_;

		};
	}	//	ipt
}


#endif
