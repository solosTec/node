/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "server.h"
#include "session.h"

namespace node 
{
	namespace imega
	{
		server::server(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, std::chrono::seconds timeout
			, bool use_global_pwd
			, std::string const& global_pwd)
		: server_stub(mux, logger, bus, timeout)
			, use_global_pwd_(use_global_pwd)
			, global_pwd_(global_pwd)
		{}


		cyng::object server::make_client(boost::uuids::uuid tag, boost::asio::ip::tcp::socket socket)
		{
			return make_session(std::move(socket)
				, mux_
				, logger_
				, bus_
				, tag
				, timeout_
				, use_global_pwd_
				, global_pwd_);
		}

		bool server::close_connection(boost::uuids::uuid tag, cyng::object obj)
		{
			//const_cast<connection*>(cyng::object_cast<connection>(conn.second))->close();
			//using connection_t = connection_stub<session, serializer>;

			const auto ptr = cyng::object_cast<session>(obj);
			if (ptr != nullptr)	const_cast<session*>(ptr)->close();
			return (ptr != nullptr);
		}

		void server::start_client(cyng::object obj)
		{
			//const_cast<connection*>(cyng::object_cast<connection>((*r.first).second))->start();
			//using connection_t = connection_stub<session, serializer>;

			const auto ptr = cyng::object_cast<session>(obj);
			if (ptr != nullptr)	const_cast<session*>(ptr)->start();
		}

		void server::propagate(cyng::object obj, cyng::vector_t&& prg)
		{
			//cyng::object_cast<connection>((*pos).second)->session_.vm_.async_run(std::move(prg));
			//using connection_t = connection_stub<session, serializer>;

			const auto ptr = cyng::object_cast<session>(obj);
			if (ptr != nullptr) const_cast<session*>(ptr)->vm().async_run(std::move(prg));
		}
	}
}



