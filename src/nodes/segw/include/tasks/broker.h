/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_BROKER_H
#define SMF_SEGW_TASK_BROKER_H

#include <cfg.h>
#include <config/cfg_broker.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>

namespace smf {

	/**
	 * connect to MDM system
	 * https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/example/cpp03/timeouts/async_tcp_client.cpp
	 */
	class broker
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(cyng::eod)>,
			std::function<void(cyng::buffer_t)>,
			std::function<void()>
		>;

	public:
		broker(std::weak_ptr<cyng::channel>
			, cyng::controller& ctl
			, cyng::logger
			, target const&
			, std::chrono::seconds
			, bool login);

	private:
		void stop(cyng::eod);
		void send(cyng::buffer_t);
		void start();

		void connect(boost::asio::ip::tcp::resolver::results_type endpoints);
		void start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
		void check_deadline();
		void handle_connect(const boost::system::error_code& ec,
			boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
		void do_read();
		void handle_read(const boost::system::error_code& ec, std::size_t n);
		void do_write();
		void handle_write(const boost::system::error_code& ec);

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		cyng::controller& ctl_;
		cyng::logger logger_;
		target const target_;
		std::chrono::seconds const timeout_;
		bool const login_;

		bool stopped_;
		boost::asio::ip::tcp::resolver::results_type endpoints_;
		boost::asio::ip::tcp::socket socket_;
		std::string input_buffer_;
		boost::asio::steady_timer deadline_;
		boost::asio::steady_timer heartbeat_timer_;

		std::deque<cyng::buffer_t>	buffer_write_;

	};
}

#endif