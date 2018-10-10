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

namespace node
{
	namespace https
	{
		connections::connections(cyng::logging::log_ptr logger
			, cyng::controller& vm
			, std::string const& doc_root)
		: logger_(logger)
			, vm_(vm)
			, doc_root_(doc_root)
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
				, doc_root_);

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
				, doc_root_);

			auto sp = const_cast<plain_session*>(cyng::object_cast<plain_session>(obj));
			BOOST_ASSERT(sp != nullptr);
			if (sp != nullptr) {
				sessions_[HTTP_PLAIN].emplace(sp->tag(), obj);
				sp->run(obj);
			}
		}

		void connections::upgrade(boost::uuids::uuid tag, boost::asio::ip::tcp::socket socket, boost::beast::http::request<boost::beast::http::string_body> req)
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
					std::lock(mutex_[HTTP_PLAIN], mutex_[SOCKET_PLAIN]);
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
					std::lock(mutex_[HTTP_SSL], mutex_[SOCKET_SSL]);
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

		//
		//	connection manager interface
		//

		bool connections::ws_msg(boost::uuids::uuid tag, std::string const& msg)
		{
			//
			//	lock both web-socket container
			//
			std::lock(mutex_[SOCKET_PLAIN], mutex_[SOCKET_SSL]);
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
			std::lock(mutex_[HTTP_PLAIN], mutex_[HTTP_SSL]);
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
			std::lock(mutex_[SOCKET_PLAIN], mutex_[SOCKET_SSL]);
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
			//	lock both web-socket container
			//
			std::lock(mutex_[SOCKET_PLAIN], mutex_[SOCKET_SSL]);
			cyng::async::lock_guard<cyng::async::shared_mutex> lk1(mutex_[SOCKET_PLAIN], cyng::async::adopt_lock);
			cyng::async::lock_guard<cyng::async::shared_mutex> lk2(mutex_[SOCKET_SSL], cyng::async::adopt_lock);

			auto range = listener_plain_.equal_range(channel);
			for (auto pos = range.first; pos != range.second; ++pos)
			{
				auto wp = const_cast<plain_websocket*>(cyng::object_cast<plain_websocket>(pos->second));
				if (wp)	wp->write(msg);
			}

			range = listener_ssl_.equal_range(channel);
			for (auto pos = range.first; pos != range.second; ++pos)
			{
				auto wp = const_cast<ssl_websocket*>(cyng::object_cast<ssl_websocket>(pos->second));
				if (wp)	wp->write(msg);
			}
		}

	}
}

