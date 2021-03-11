/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_TASK_WMBUS_SERVER_H
#define SMF_TASK_WMBUS_SERVER_H

//#include <smf/http/server.h>

#include <cyng/obj/intrinsics/eod.h>
#include <cyng/log/logger.h>

#include <tuple>
#include <functional>
#include <string>
#include <memory>

#include <boost/uuid/uuid.hpp>

namespace cyng {
	template <typename T >
	class task;

	class channel;
}
namespace smf {

	class wmbus_server
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(cyng::eod)>,
			std::function<void(boost::asio::ip::tcp::endpoint)>
		>;

	public:
		wmbus_server(std::weak_ptr<cyng::channel>
			, boost::asio::io_context&
			, boost::uuids::uuid tag
			, cyng::logger);
		~wmbus_server();

		void stop(cyng::eod);
		void listen(boost::asio::ip::tcp::endpoint);

	private:
		/**
		 * incoming connection
		 */
		void accept(boost::asio::ip::tcp::socket&&);


	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;
		boost::uuids::uuid const tag_;
		cyng::logger logger_;

	};

}

#endif
