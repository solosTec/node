/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MAIN_TASK_SERVER_H
#define SMF_MAIN_TASK_SERVER_H

#include <db.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>
#include <cyng/task/controller.h>
#include <cyng/vm/mesh.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

	class session;
	class server
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(boost::asio::ip::tcp::endpoint ep)>,
			std::function<void(cyng::eod)>
		>;

		friend class session;

	public:
		server(std::weak_ptr<cyng::channel>
			, cyng::controller& ctl
			, boost::uuids::uuid tag
			, cyng::logger
			, std::string const& account
			, std::string const& pwd
			, std::string const& salt
			, std::chrono::seconds monitor
			, cyng::param_map_t const&);
		~server();

	private:
		void start(boost::asio::ip::tcp::endpoint ep);
		void do_accept();
		void stop(cyng::eod);

		void pty_connect(boost::uuids::uuid tag
			, std::string msisdn);

		void pty_register(boost::uuids::uuid
			, std::string
			, std::uint16_t
			, std::uint8_t);

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		boost::uuids::uuid const tag_;
		cyng::logger logger_;

		std::string const account_;
		std::string const pwd_;
		std::string const salt_;
		std::chrono::seconds const monitor_;

		boost::asio::ip::tcp::acceptor acceptor_;
		std::uint64_t session_counter_;

		cyng::mesh fabric_;

		/**
		 *  data cache
		 */
		cyng::store store_;
		db cache_;
	};

}

#endif
