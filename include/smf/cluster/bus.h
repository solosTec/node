/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_CLUSTER_BUS_H
#define SMF_CLUSTER_BUS_H


#include <smf/cluster/config.h>

#include <cyng/log/logger.h>
//#include <cyng/vm/mesh.h>
#include <cyng/obj/intrinsics/buffer.h>

namespace smf
{
	/**
	 * The TCP/IP behaviour is modelled after the async_tcp_client.cpp" example
	 * in the asio C++11 example folder (libs/asio/example/cpp11/timeouts/async_tcp_client.cpp)
	 */
	class bus
	{
		enum class state {
			START,
			WAIT,
			CONNECTED,
			STOPPED,
		} state_;
	public:
		bus(boost::asio::io_context& ctx
			, cyng::logger
			, toggle::server_vec_t&& cfg
			, std::string const& node_name);

		void start();
		void stop();

	private:
		void reset();
		void connect(boost::asio::ip::tcp::resolver::results_type endpoints);
		void start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
		void handle_connect(boost::system::error_code const&,
			boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
		void do_read();
		void do_write();
		void handle_read(boost::system::error_code const&, std::size_t n);
		void handle_write(boost::system::error_code const&);
		void check_deadline(boost::system::error_code const&);

		constexpr bool is_stopped() const {
			return state_ == state::STOPPED;
		}
	private:
		boost::asio::io_context& ctx_;
		cyng::logger logger_;
		toggle const tgl_;
		std::string const node_name_;

		//bool stopped_;
		boost::asio::ip::tcp::resolver::results_type endpoints_;
		boost::asio::ip::tcp::socket socket_;
		boost::asio::steady_timer timer_;
		std::deque<cyng::buffer_t>	buffer_write_;
		std::array<char, 2048>	input_buffer_;
	};
}


#endif
