/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "server.h"
#include "session.h"
#include <smf/ipt/scramble_key_io.hpp>

namespace node 
{
	namespace ipt
	{
		server::server(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, std::chrono::seconds timeout
			, scramble_key const& sk
			, uint16_t watchdog
			, bool sml_log)
		: server_stub(mux, logger, bus, timeout)
			, sk_(sk)
			, watchdog_(watchdog)
			, sml_log_(sml_log)
		{
			//
			//	client/server functions
			//
			bus_->vm_.register_function("client.req.gateway", 12, [&](cyng::context& ctx) { this->server_stub::client_propagate(ctx); });

			//
			//	statistical data
			//
			bus_->vm_.async_run(cyng::generate_invoke("log.msg.debug", cyng::invoke("lib.size"), " callbacks registered"));
		}

		cyng::object server::make_client(boost::uuids::uuid tag, boost::asio::ip::tcp::socket socket)
		{
			return make_session(std::move(socket)
				, mux_
				, logger_
				, bus_
				, tag
				, timeout_
				, sk_
				, watchdog_
				, sml_log_);
		}

		bool server::close_connection(boost::uuids::uuid tag, cyng::object obj)
		{
			const auto ptr = cyng::object_cast<session>(obj);
			if (ptr != nullptr)	const_cast<session*>(ptr)->close();
			return (ptr != nullptr);
		}

		void server::start_client(cyng::object obj)
		{
			const auto ptr = cyng::object_cast<session>(obj);
			if (ptr != nullptr)	const_cast<session*>(ptr)->start();
		}

		void server::propagate(cyng::object obj, cyng::vector_t&& prg)
		{
			const auto ptr = cyng::object_cast<session>(obj);
			if (ptr != nullptr) const_cast<session*>(ptr)->vm().async_run(std::move(prg));
		}
	}
}



