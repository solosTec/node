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
#include <cyng/vm/mesh.h>

namespace smf
{
	class bus
	{
	public:
		bus(cyng::mesh& mesh, cyng::logger, toggle::server_vec_t&& cfg);

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

	private:
		cyng::mesh& fabric_;
		cyng::logger logger_;
		toggle const tgl_;
		bool stopped_;
		boost::asio::ip::tcp::resolver::results_type endpoints_;
		boost::asio::ip::tcp::socket socket_;
		boost::asio::steady_timer deadline_;
		std::deque<cyng::buffer_t>	buffer_write_;
		std::array<char, 2048>	input_buffer_;
	};
}


#endif
