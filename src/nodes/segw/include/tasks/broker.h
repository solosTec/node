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
			std::function<void(cyng::buffer_t)>,
			std::function<void(std::chrono::seconds)>,
			std::function<void(cyng::eod)>
		>;

		enum class state {
			START,
			WAIT,
			CONNECTED,
			STOPPED,
		} state_;

	public:
		broker(std::weak_ptr<cyng::channel>
			, cyng::controller& ctl
			, cyng::logger
			, target const&
			, bool login);

	private:
		void stop(cyng::eod);
		void start();

		/**
		 * incoming raw data from serial interface
		 */
		void receive(cyng::buffer_t);

		void connect(boost::asio::ip::tcp::resolver::results_type endpoints);
		void start_connect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
		void handle_connect(const boost::system::error_code& ec,
			boost::asio::ip::tcp::resolver::results_type::iterator endpoint_iter);
		void do_read();
		void handle_read(const boost::system::error_code& ec, std::size_t n);
		void do_write();
		void handle_write(const boost::system::error_code& ec);
		//void check_deadline(const boost::system::error_code& ec);
		void check_status(std::chrono::seconds);

		constexpr bool is_stopped() const {
			return state_ == state::STOPPED;
		}
		constexpr bool is_connected() const {
			return state_ == state::CONNECTED;
		}

		void reset();

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		cyng::controller& ctl_;
		cyng::logger logger_;
		target const target_;
		bool const login_;

		boost::asio::ip::tcp::resolver::results_type endpoints_;
		boost::asio::ip::tcp::socket socket_;
		//boost::asio::steady_timer timer_;
		boost::asio::io_context::strand dispatcher_;

		std::string input_buffer_;
		std::deque<cyng::buffer_t>	buffer_write_;

	};
}

#endif