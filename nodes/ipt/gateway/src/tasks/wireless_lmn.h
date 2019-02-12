/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_WIRELESS_LMN_H
#define NODE_WIRELESS_LMN_H


#include <smf/ipt/bus.h>
#include <cyng/log.h>
#include <cyng/store/db.h>
#include <cyng/async/mux.h>

namespace node
{
	/**
	 * wireless M-Bus
	 */
	class wireless_LMN
	{
	public:
		using signatures_t = std::tuple<>;

	public:
		wireless_LMN(cyng::async::base_task* btp
			, cyng::logging::log_ptr
			, cyng::store::db& config_db
			, cyng::controller& vm
			, std::string port
			, std::uint8_t databits
			, std::uint8_t paritybit
			, std::uint8_t rtscts
			, std::uint8_t stopbits
			, std::uint32_t speed);

		cyng::continuation run();
		void stop();

	private:

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * configuration db
		 */
		cyng::store::db& config_db_;

		/**
		 * execution engine
		 */
		cyng::controller& vm_;

		std::string const port_;
		std::uint8_t const databits_;
		std::uint8_t const paritybit_;
		std::uint8_t const rtscts_;
		std::uint8_t const stopbits_;
		std::uint32_t const speed_;

	};
}

#endif