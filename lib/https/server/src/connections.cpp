/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/https/srv/connections.h>
#include <smf/https/srv/detector.h>
#include <smf/https/srv/session.h>
#include <smf/https/srv/websocket.h>
#include <cyng/object_cast.hpp>
#include <cyng/vm/controller.h>
#include <cyng/compatibility/async.h>

namespace node
{
	namespace https
	{
		connections::connections(cyng::logging::log_ptr logger
			, cyng::controller& vm
			, std::string const& doc_root
			, auth_dirs const& ad)
		: logger_(logger)
			, vm_(vm)
			, doc_root_(doc_root)
			, auth_dirs_(ad)
			, uidgen_()
			, sessions_()
			, mutex_()
			, listener_plain_()
			, listener_ssl_()
		{}

		cyng::controller& connections::vm()
		{
			return vm_;
		}

		void connections::create_session(boost::asio::ip::tcp::socket socket
			, boost::asio::ssl::context& ctx)
		{
			std::make_shared<detector>(logger_
				, *this
				, std::move(socket)
				, ctx)->run();
		}

		void connections::add_ssl_session(boost::asio::ip::tcp::socket socket, boost::asio::ssl::context& ctx, boost::beast::flat_buffer buffer)
		{
			auto obj = cyng::make_object<ssl_session>(logger_
				, *this
				, uidgen_()
				, std::move(socket)
				, ctx
				, std::move(buffer)
				, doc_root_
				, auth_dirs_);

			auto sp = const_cast<ssl_session*>(cyng::object_cast<ssl_session>(obj));
			BOOST_ASSERT(sp != nullptr);
			if (sp != nullptr) {
				sessions_[HTTP_SSL].emplace(sp->tag(), obj);
				sp->run(obj);
			}
		}

		void connections::add_plain_session(boost::asio::ip::tcp::socket socket, boost::beast::flat_buffer buffer)
		{
			auto obj = cyng::make_object<plain_session>(logger_
				, *this
				, uidgen_()
				, std::move(socket)
				, std::move(buffer)
				, doc_root_
				, auth_dirs_);

			auto sp = const_cast<plain_session*>(cyng::object_cast<plain_session>(obj));
			BOOST_ASSERT(sp != nullptr);
			if (sp != nullptr) {
				sessions_[HTTP_PLAIN].emplace(sp->tag(), obj);
				sp->run(obj);
			}
		}

		void connections::upgrade(boost::uuids::uuid tag
			, boost::asio::ip::tcp::socket socket
			, boost::beast::http::request<boost::beast::http::string_body> req)
		{
			//	plain websocket
			auto obj = create_wsocket(tag, std::move(socket));
			auto wp = const_cast<plain_websocket*>(cyng::object_cast<plain_websocket>(obj));
			BOOST_ASSERT(wp != nullptr);
			if (wp != nullptr) {

				CYNG_LOG_DEBUG(logger_, "upgrade plain session " << tag);

				//
				//	lock HTTP_PLAIN and SOCKET_PLAIN 
				//
				{
					cyng::async::lock(mutex_[HTTP_PLAIN], mutex_[SOCKET_PLAIN]);
					cyng::async::lock_guard<cyng::async::shared_mutex> lk1(mutex_[HTTP_PLAIN], cyng::async::adopt_lock);
					cyng::async::lock_guard<cyng::async::shared_mutex> lk2(mutex_[SOCKET_PLAIN], cyng::async::adopt_lock);

					const auto size = sessions_[HTTP_PLAIN].erase(tag);
					BOOST_ASSERT(size == 1);
					sessions_[SOCKET_PLAIN].emplace(tag, obj);
				}	//	unlock

				wp->run(obj, std::move(req));
			}
		}

		void connections::upgrade(boost::uuids::uuid tag, boost::beast::ssl_stream<boost::asio::ip::tcp::socket> stream, boost::beast::http::request<boost::beast::http::string_body> req)
		{
			//	ssl websocket
			auto obj = create_wsocket(tag, std::move(stream));
			auto wp = const_cast<ssl_websocket*>(cyng::object_cast<ssl_websocket>(obj));
			BOOST_ASSERT(wp != nullptr);
			if (wp != nullptr) {

				CYNG_LOG_DEBUG(logger_, "upgrade ssl session " << tag);

				//
				//	lock HTTP_SSL and SOCKET_SSL 
				//
				{
					cyng::async::lock(mutex_[HTTP_SSL], mutex_[SOCKET_SSL]);
					cyng::async::lock_guard<cyng::async::shared_mutex> lk1(mutex_[HTTP_SSL], cyng::async::adopt_lock);
					cyng::async::lock_guard<cyng::async::shared_mutex> lk2(mutex_[SOCKET_SSL], cyng::async::adopt_lock);

					const auto size = sessions_[HTTP_SSL].erase(tag);
					BOOST_ASSERT(size == 1);
					sessions_[SOCKET_SSL].emplace(tag, obj);
				}	//	unlock

				wp->run(obj, std::move(req));
			}
		}

		cyng::object connections::create_wsocket(boost::uuids::uuid tag, boost::asio::ip::tcp::socket socket)
		{
			return cyng::make_object<plain_websocket>(logger_, *this, tag, std::move(socket));
		}

		cyng::object connections::create_wsocket(boost::uuids::uuid tag, boost::beast::ssl_stream<boost::asio::ip::tcp::socket> stream)
		{
			return cyng::make_object<ssl_websocket>(logger_, *this, tag, std::move(stream));
		}

		std::pair<cyng::object, connections::session_type> connections::find_http(boost::uuids::uuid tag) const
		{
			auto pos = sessions_[HTTP_PLAIN].find(tag);
			if (pos != sessions_[HTTP_PLAIN].end()) {
				return std::make_pair(pos->second, HTTP_PLAIN);
			}

			pos = sessions_[HTTP_SSL].find(tag);
			if (pos != sessions_[HTTP_SSL].end()) {
				return std::make_pair(pos->second, HTTP_SSL);
			}

			//
			//	not found
			//
			CYNG_LOG_WARNING(logger_, "cannot find http " << tag);
			return std::make_pair(cyng::make_object(), NO_SESSION);

		}

		std::pair<cyng::object, connections::session_type> connections::find_socket(boost::uuids::uuid tag) const
		{
			auto pos = sessions_[SOCKET_PLAIN].find(tag);
			if (pos != sessions_[SOCKET_PLAIN].end()) {
				return std::make_pair(pos->second, SOCKET_PLAIN);
			}

			pos = sessions_[SOCKET_SSL].find(tag);
			if (pos != sessions_[SOCKET_SSL].end()) {
				return std::make_pair(pos->second, SOCKET_SSL);
			}

			//
			//	not found
			//
			CYNG_LOG_WARNING(logger_, "cannot find ws " << tag);
			return std::make_pair(cyng::make_object(), NO_SESSION);
		}

		cyng::object connections::stop_session(boost::uuids::uuid tag)
		{
			//
			//	unique lock: HTTP_PLAIN
			//
			{
				cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[HTTP_PLAIN]);
				auto pos = sessions_[HTTP_PLAIN].find(tag);
				if (pos != sessions_[HTTP_PLAIN].end()) {
					//
					//	get a session reference
					//
					CYNG_LOG_INFO(logger_, "remove HTTP_PLAIN " << tag);
					auto obj = pos->second;
					const_cast<plain_session*>(cyng::object_cast<plain_session>(obj))->do_eof(obj);
					sessions_[HTTP_PLAIN].erase(pos);
					return obj;
				}
			}

			//
			//	unique lock: HTTP_SSL
			//
			{
				cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[HTTP_SSL]);

				auto pos = sessions_[HTTP_SSL].find(tag);
				if (pos != sessions_[HTTP_SSL].end()) {
					CYNG_LOG_INFO(logger_, "remove HTTP_SSL " << tag);
					auto obj = pos->second;
					const_cast<ssl_session*>(cyng::object_cast<ssl_session>(obj))->do_eof(obj);
					sessions_[HTTP_SSL].erase(pos);
					return obj;
				}
			}

			return cyng::make_object();
		}

		cyng::object connections::stop_ws(boost::uuids::uuid tag)
		{
			//
			//	unique lock: SOCKET_PLAIN
			//
			{
				cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[SOCKET_PLAIN]);

				//
				//	Clean up listener list
				//	Remove all listener with the same tag as *wsp.
				//
				for (auto pos = listener_plain_.begin(); pos != listener_plain_.end(); )
				{
					if (cyng::object_cast<plain_websocket>(pos->second)->tag() == tag) {
						pos = listener_plain_.erase(pos);
					}
					else {
						++pos;
					}
				}

				//
				//	remove from ws-session list
				//
				auto pos = sessions_[SOCKET_PLAIN].find(tag);
				if (pos != sessions_[SOCKET_PLAIN].end()) {

					//
					//	get a session reference
					//
					auto obj = pos->second;
					//const_cast<plain_websocket*>(cyng::object_cast<plain_websocket>(obj))->do_eof(obj);

					sessions_[SOCKET_PLAIN].erase(pos);
					return obj;
				}
			}

			//
			//	unique lock: SOCKET_SSL
			//
			{
				cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[SOCKET_SSL]);

				//
				//	Clean up listener list
				//	Remove all listener with the same tag as *wsp.
				//
				for (auto pos = listener_ssl_.begin(); pos != listener_ssl_.end(); )
				{
					if (cyng::object_cast<ssl_websocket>(pos->second)->tag() == tag) {
						pos = listener_ssl_.erase(pos);
					}
					else {
						++pos;
					}
				}

				//
				//	remove from ws-session list
				//
				auto pos = sessions_[SOCKET_SSL].find(tag);
				if (pos != sessions_[SOCKET_SSL].end()) {

					//
					//	get a session reference
					//
					auto obj = pos->second;
					//const_cast<ssl_websocket*>(cyng::object_cast<ssl_websocket>(obj))->do_close();

					sessions_[SOCKET_SSL].erase(pos);
					return obj;
				}
			}

			return cyng::make_object();
		}

		void connections::stop_all()
		{
			{
				//
				//	unique lock: HTTP_PLAIN
				//
				cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[HTTP_PLAIN]);

				CYNG_LOG_INFO(logger_, "stop " << sessions_[HTTP_PLAIN].size() << " HTTP_PLAIN sessions");
				for (auto s : sessions_[HTTP_PLAIN])
				{
					auto ptr = cyng::object_cast<plain_session>(s.second);
					if (ptr)	const_cast<plain_session*>(ptr)->do_eof(s.second);
				}
			}
			{
				//
				//	unique lock: HTTP_SSL
				//
				cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[HTTP_SSL]);

				CYNG_LOG_INFO(logger_, "stop " << sessions_[HTTP_SSL].size() << " HTTP_SSL sessions");
				for (auto s : sessions_[HTTP_SSL])
				{
					auto ptr = cyng::object_cast<ssl_session>(s.second);
					if (ptr)	const_cast<ssl_session*>(ptr)->do_eof(s.second);
				}
			}

			{
				//
				//	unique lock: SOCKET_PLAIN
				//
				cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[SOCKET_PLAIN]);

				CYNG_LOG_INFO(logger_, "stop " << sessions_[SOCKET_PLAIN].size() << " SOCKET_PLAIN websockets");
				for (auto & ws : sessions_[SOCKET_PLAIN])
				{
					auto ptr = cyng::object_cast<plain_websocket>(ws.second);
					//if (ptr)	const_cast<plain_websocket*>(ptr)->do_shutdown();
				}
			}
			{
				//
				//	unique lock: SOCKET_SSL
				//
				cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[SOCKET_SSL]);

				CYNG_LOG_INFO(logger_, "stop " << sessions_[SOCKET_SSL].size() << " SOCKET_SSL websockets");
				for (auto & ws : sessions_[SOCKET_SSL])
				{
					auto ptr = cyng::object_cast<ssl_websocket>(ws.second);
					//if (ptr)	const_cast<plain_websocket*>(ptr)->do_shutdown();
				}
			}

			//
			//	clear all maps
			//
			sessions_[HTTP_PLAIN].clear();
			sessions_[HTTP_SSL].clear();
			sessions_[SOCKET_PLAIN].clear();
			sessions_[SOCKET_SSL].clear();
			listener_plain_.clear();
			listener_ssl_.clear();
		}

		//
		//	connection manager interface
		//

		bool connections::ws_msg(boost::uuids::uuid tag, std::string const& msg)
		{
			//
			//	lock both web-socket container
			//
			cyng::async::lock(mutex_[SOCKET_PLAIN], mutex_[SOCKET_SSL]);
			cyng::async::lock_guard<cyng::async::shared_mutex> lk1(mutex_[SOCKET_PLAIN], cyng::async::adopt_lock);
			cyng::async::lock_guard<cyng::async::shared_mutex> lk2(mutex_[SOCKET_SSL], cyng::async::adopt_lock);

			auto res = find_socket(tag);
			switch (res.second) {
			case SOCKET_PLAIN:
				const_cast<plain_websocket*>(cyng::object_cast<plain_websocket>(res.first))->write(msg);
				return true;
			case SOCKET_SSL:
				const_cast<ssl_websocket*>(cyng::object_cast<ssl_websocket>(res.first))->write(msg);
				return true;
			default:
				break;
			}

			return false;
		}


		bool connections::http_moved(boost::uuids::uuid tag, std::string const& location)
		{
			//
			//	lock both http container
			//
			cyng::async::lock(mutex_[HTTP_PLAIN], mutex_[HTTP_SSL]);
			cyng::async::lock_guard<cyng::async::shared_mutex> lk1(mutex_[HTTP_PLAIN], cyng::async::adopt_lock);
			cyng::async::lock_guard<cyng::async::shared_mutex> lk2(mutex_[HTTP_SSL], cyng::async::adopt_lock);

			auto res = find_http(tag);
			switch (res.second) {
			case HTTP_PLAIN:
				const_cast<plain_session*>(cyng::object_cast<plain_session>(res.first))->temporary_redirect(res.first, location);
				return true;
			case HTTP_SSL:
				const_cast<ssl_session*>(cyng::object_cast<ssl_session>(res.first))->temporary_redirect(res.first, location);
				return true;
			default:
				break;
			}
			return false;
		}

		bool connections::add_channel(boost::uuids::uuid tag, std::string const& channel)
		{
			//
			//	lock both web-socket container
			//
			cyng::async::lock(mutex_[SOCKET_PLAIN], mutex_[SOCKET_SSL]);
			cyng::async::lock_guard<cyng::async::shared_mutex> lk1(mutex_[SOCKET_PLAIN], cyng::async::adopt_lock);
			cyng::async::lock_guard<cyng::async::shared_mutex> lk2(mutex_[SOCKET_SSL], cyng::async::adopt_lock);

			auto res = find_socket(tag);
			switch (res.second) {
			case SOCKET_PLAIN:
				//
				//	add as listener
				//	ToDo: lock listener_plain_
				//
				listener_plain_.emplace(channel, res.first);
				return true;
			case SOCKET_SSL:
				//
				//	add as listener
				//	ToDo: lock listener_ssl_
				//
				listener_ssl_.emplace(channel, res.first);
				return true;
			default:
				break;
			}

			return false;
		}

		void connections::push_event(std::string const& channel, std::string const& msg)
		{
			//
			//	same lock as used for web-socket container
			//	lock both web-socket container
			//
			cyng::async::lock(mutex_[SOCKET_PLAIN], mutex_[SOCKET_SSL]);
			cyng::async::lock_guard<cyng::async::shared_mutex> lk1(mutex_[SOCKET_PLAIN], cyng::async::adopt_lock);
			cyng::async::lock_guard<cyng::async::shared_mutex> lk2(mutex_[SOCKET_SSL], cyng::async::adopt_lock);

			auto range = listener_plain_.equal_range(channel);
			for (auto pos = range.first; pos != range.second; ++pos)
			{
				auto wp = const_cast<plain_websocket*>(cyng::object_cast<plain_websocket>(pos->second));
				if (wp)	wp->write(msg);
				else {
					CYNG_LOG_ERROR(logger_, "plain_websocket of channel " << channel << " is NULL");
					pos = listener_plain_.erase(pos);
				}
			}

			range = listener_ssl_.equal_range(channel);
			for (auto pos = range.first; pos != range.second; ++pos)
			{
				auto wp = const_cast<ssl_websocket*>(cyng::object_cast<ssl_websocket>(pos->second));
				if (wp)	wp->write(msg);
				else {
					CYNG_LOG_ERROR(logger_, "ssl_websocket of channel " << channel << " is NULL");
					pos = listener_plain_.erase(pos);
				}
			}
		}

		void connections::trigger_download(boost::uuids::uuid tag, std::string const& filename, std::string const& attachment)
		{
			//
			//	lock both http container
			//
			cyng::async::lock(mutex_[HTTP_PLAIN], mutex_[HTTP_SSL]);
			cyng::async::lock_guard<cyng::async::shared_mutex> lk1(mutex_[HTTP_PLAIN], cyng::async::adopt_lock);
			cyng::async::lock_guard<cyng::async::shared_mutex> lk2(mutex_[HTTP_SSL], cyng::async::adopt_lock);

			auto res = find_http(tag);
			switch (res.second) {
			case HTTP_PLAIN:
				const_cast<plain_session*>(cyng::object_cast<plain_session>(res.first))->trigger_download(res.first, filename, attachment);
				break;
			case HTTP_SSL:
				const_cast<ssl_session*>(cyng::object_cast<ssl_session>(res.first))->trigger_download(res.first, filename, attachment);
				break;
			default:
				break;
			}

			//cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[HTTP_PLAIN]);
			//auto pos = sessions_[HTTP_PLAIN].find(tag);
			//if (pos != sessions_[HTTP_PLAIN].end()) {
			//	auto ptr = cyng::object_cast<session>(pos->second);
			//	if (ptr != nullptr) {
			//		const_cast<session*>(ptr)->trigger_download(filename, attachment);
			//	}
			//}
		}

	}
}

