/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include <smf/http/srv/connections.h>
#include <cyng/object_cast.hpp>

namespace node 
{
	namespace http
	{
		connection_manager::connection_manager(cyng::logging::log_ptr logger)
			: logger_(logger)
			, connections_()
			, ws_()
		{}

		void connection_manager::start(session_ptr sp)
		{
			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::mutex> lock(mutex_);

			connections_.insert(sp);
			sp->run();
		}

		websocket_session const* connection_manager::upgrade(session_ptr sp)
		{
			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::mutex> lock(mutex_);

			//
			//	create new websocket
			//
			auto res = ws_.emplace(sp->tag_
				, make_websocket(sp->logger_
					, *this
					, std::move(sp->socket_)
					, sp->bus_
					, sp->tag_));

			//
			//	remove HTTP session
			//
			connections_.erase(sp);
			sp.reset();

			return (res.second)
				? cyng::object_cast<websocket_session>(res.first->second)
				: nullptr
				;
		}

		void connection_manager::stop(session_ptr sp)
		{
			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::mutex> lock(mutex_);

			connections_.erase(sp);
			sp->do_close();
		}

		bool connection_manager::stop(websocket_session const* wsp)
		{
			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::mutex> lock(mutex_);
			return ws_.erase(wsp->tag()) != 0;
		}

		void connection_manager::stop_all()
		{
			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::mutex> lock(mutex_);

			CYNG_LOG_INFO(logger_, "stop " << connections_.size() << " sessions");
			for (auto sp : connections_)
			{
				sp->do_close();
			}
			CYNG_LOG_INFO(logger_, "stop " << ws_.size() << " websockets");
			for (auto wssp : ws_)
			{
				//wssp->do_close();
			}
			connections_.clear();
			ws_.clear();
		}

		void connection_manager::send_ws(boost::uuids::uuid tag, cyng::vector_t&& prg)
		{
			//
			//	shared lock is buggy on visual studio 2017
			//
			//cyng::async::shared_lock<cyng::async::mutex> lock(mutex_);
			cyng::async::unique_lock<cyng::async::mutex> lock(mutex_);

			auto pos = ws_.find(tag);
			if (pos != ws_.end())
			{
				auto ptr = cyng::object_cast<websocket_session>(pos->second);
				const_cast<websocket_session*>(ptr)->run(std::move(prg));
			}
			
		}

	}
}




