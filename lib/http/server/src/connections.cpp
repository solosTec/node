/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include <smf/http/srv/connections.h>
#include <cyng/object_cast.hpp>
#include <cyng/vm/generator.h>

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node 
{
	namespace http
	{
		connection_manager::connection_manager(cyng::logging::log_ptr logger, bus::shared_type bus)
			: logger_(logger)
			, connections_()
			, ws_()
			, listener_()
			, mutex_()
		{
			bus->vm_.run(cyng::register_function("ws.push", 1, std::bind(&connection_manager::push_ws, this, std::placeholders::_1)));
		}

		void connection_manager::start(session_ptr sp)
		{
			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_);

			connections_.insert(sp);
			sp->run();
		}

		websocket_session const* connection_manager::upgrade(session_ptr sp)
		{
			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_);

			//
			//	create new websocket
			//
			auto res = ws_.emplace(sp->tag_
				, make_websocket(sp->logger_
					, *this
					, std::move(sp->socket_)
					, sp->bus_
					, sp->tag_));	//	share same tag

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
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_);

			connections_.erase(sp);
			sp->do_close();
		}

		bool connection_manager::stop(websocket_session const* wsp)
		{
			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_);

			//
			//	Clean up listener list
			//	Remove all listener with the same tag as *wsp.
			//
			for (auto pos = listener_.begin(); pos != listener_.end(); )
			{
				if (cyng::object_cast<websocket_session>(pos->second)->tag() == wsp->tag())
				{
					pos = listener_.erase(pos);
				}
				else
				{
					++pos;
				}
			}

			return ws_.erase(wsp->tag()) != 0;
		}

		void connection_manager::stop_all()
		{
			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_);

			CYNG_LOG_INFO(logger_, "stop " << connections_.size() << " sessions");
			for (auto sp : connections_)
			{
				sp->do_close();
			}
			CYNG_LOG_INFO(logger_, "stop " << ws_.size() << " websockets");
            for (auto & wssp : ws_)
			{
                auto ptr = cyng::object_cast<websocket_session>(wssp.second);
                if (ptr)
                {
                    const_cast<websocket_session*>(ptr)->do_shutdown();
                }
			}
			connections_.clear();
			ws_.clear();
			listener_.clear();
		}

		//void connection_manager::run_on_ws(boost::uuids::uuid tag, cyng::vector_t&& prg)
		//{
		//	//
		//	//	shared lock 
		//	//
		//	cyng::async::shared_lock<cyng::async::shared_mutex> lock(mutex_);

		//	auto pos = ws_.find(tag);
		//	if (pos != ws_.end())
		//	{
		//		auto ptr = cyng::object_cast<websocket_session>(pos->second);
		//		if (ptr)
		//		{
		//			const_cast<websocket_session*>(ptr)->run(std::move(prg));
		//		}
		//	}
		//	
		//}

		bool connection_manager::send_msg(boost::uuids::uuid tag, std::string const& msg)
		{
			//
			//	shared lock 
			//
			cyng::async::shared_lock<cyng::async::shared_mutex> lock(mutex_);

			auto pos = ws_.find(tag);
			if (pos != ws_.end())
			{
				auto ptr = cyng::object_cast<websocket_session>(pos->second);
				if (ptr)
				{
					const_cast<websocket_session*>(ptr)->send_msg(msg);
					return true;
				}
			}
			return false;
		}


		bool connection_manager::add_channel(boost::uuids::uuid tag, std::string const& channel)
		{
			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_);

			auto pos = ws_.find(tag);
			if (pos != ws_.end())
			{
				listener_.emplace(channel, pos->second);
				return true;
			}

			return false;
		}

		//void connection_manager::process_event(std::string const& channel, cyng::vector_t&& prg)
		//{
		//	//
		//	//	shared lock 
		//	//
		//	cyng::async::shared_lock<cyng::async::shared_mutex> lock(mutex_);
		//	auto range = listener_.equal_range(channel);
		//	for (auto pos = range.first; pos != range.second; ++pos)
		//	{
		//		auto ptr = cyng::object_cast<websocket_session>(pos->second);
  //              if (ptr)
  //              {
  //                  //
  //                  //  websocket works asynch internally
  //                  //
  //                  const_cast<websocket_session*>(ptr)->run(cyng::vector_t(prg));
  //              }
  //              else
  //              {
  //                  CYNG_LOG_ERROR(logger_, "ws of channel " << channel << " is NULL");
  //              }
		//	}
		//}
		void connection_manager::process_event(std::string const& channel, std::string const& msg)
		{
			//
			//	shared lock 
			//
			cyng::async::shared_lock<cyng::async::shared_mutex> lock(mutex_);
			auto range = listener_.equal_range(channel);
			for (auto pos = range.first; pos != range.second; ++pos)
			{
				auto ptr = cyng::object_cast<websocket_session>(pos->second);
				if (ptr)
				{
					//
					//  websocket works asynch internally
					//
					//const_cast<websocket_session*>(ptr)->run(cyng::vector_t(prg));
					const_cast<websocket_session*>(ptr)->send_msg(msg);
				}
				else
				{
					CYNG_LOG_ERROR(logger_, "ws of channel " << channel << " is NULL");
				}
			}
		}

		void connection_manager::push_ws(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			const boost::uuids::uuid tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());

			CYNG_LOG_TRACE(logger_, "push ws "
				<< tag
				<< " on stack");
			ctx.push(get_ws(tag));
		}

		cyng::object connection_manager::get_ws(boost::uuids::uuid tag) const
		{
			//
			//	shared lock 
			//
			cyng::async::shared_lock<cyng::async::shared_mutex> lock(mutex_);
			auto pos = ws_.find(tag);
			return (pos != ws_.end())
				? pos->second
				: cyng::make_object()
				;
		}


	}
}




