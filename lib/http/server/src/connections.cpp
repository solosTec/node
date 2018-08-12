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
			, sessions_()
			, ws_()
			, listener_()
			, mutex_()
		{
			bus->vm_.register_function("ws.push", 1, std::bind(&connection_manager::push_ws, this, std::placeholders::_1));
		}

		void connection_manager::start(cyng::object so)
		{
			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_);

			auto sp = cyng::object_cast<session>(so);
			if (sp != nullptr) {
				sessions_.emplace(sp->tag(), so);
				const_cast<session*>(sp)->run(so);
			}
		}

		std::pair<cyng::object, cyng::object> connection_manager::upgrade(session* sp)
		{
			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_);

			//
			//	remove HTTP session
			//
			auto pos = sessions_.find(sp->tag());
			if (pos != sessions_.end()) {

				//
				//	Hold a session reference,
				//	otherwise the session pointer gets invalid.
				//
				auto sobj = pos->second;
				sessions_.erase(pos);
				BOOST_ASSERT(cyng::object_cast<session>(sobj) == sp);

				//
				//	create new websocket
				//
				auto wobj = make_websocket(sp->logger_
					, *this
					, std::move(sp->socket_)
					, sp->bus_
					, sp->tag_);
				auto res = ws_.emplace(sp->tag_, wobj);	//	share same tag

				return std::make_pair(sobj, wobj);
			}
			return std::make_pair(cyng::make_object(), cyng::make_object());
		}

		cyng::object connection_manager::stop(session* sp)
		{
			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_);

			auto pos = sessions_.find(sp->tag());
			if (pos != sessions_.end()) {

				//
				//	get a session reference
				//
				auto obj = pos->second;
				sp->do_close();
				BOOST_ASSERT(cyng::object_cast<session>(obj) == sp);
				return obj;
			}

			return cyng::make_object();

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
				if (cyng::object_cast<websocket_session>(pos->second)->tag() == wsp->tag()) {
					pos = listener_.erase(pos);
				}
				else {
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

			CYNG_LOG_INFO(logger_, "stop " << sessions_.size() << " sessions");
			for (auto s : sessions_)
			{
				auto ptr = cyng::object_cast<session>(s.second);
				if (ptr)
				{
					const_cast<session*>(ptr)->do_close();
				}
			}

			CYNG_LOG_INFO(logger_, "stop " << ws_.size() << " websockets");
            for (auto & ws : ws_)
			{
                auto ptr = cyng::object_cast<websocket_session>(ws.second);
                if (ptr)
                {
                    const_cast<websocket_session*>(ptr)->do_shutdown();
                }
			}

			//
			//	clear all maps
			//
			sessions_.clear();
			ws_.clear();
			listener_.clear();
		}

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
				if (ptr) {
					//
					//  websocket works asynch internally
					//
					const_cast<websocket_session*>(ptr)->send_msg(msg);
				}
				else
				{
					CYNG_LOG_ERROR(logger_, "ws of channel " << channel << " is NULL");
				}
			}
		}

		void connection_manager::send_moved(boost::uuids::uuid tag, std::string const& location)
		{
			cyng::async::shared_lock<cyng::async::shared_mutex> lock(mutex_);
			auto pos = sessions_.find(tag);
			if (pos != sessions_.end()) {
				auto ptr = cyng::object_cast<session>(pos->second);
				if (ptr != nullptr) {
					const_cast<session*>(ptr)->send_moved(location);
				}
			}
		}

		void connection_manager::trigger_download(boost::uuids::uuid tag, std::string const& filename, std::string const& attachment)
		{
			cyng::async::shared_lock<cyng::async::shared_mutex> lock(mutex_);
			auto pos = sessions_.find(tag);
			if (pos != sessions_.end()) {
				auto ptr = cyng::object_cast<session>(pos->second);
				if (ptr != nullptr) {
					const_cast<session*>(ptr)->trigger_download(filename, attachment);
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




