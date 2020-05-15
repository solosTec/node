/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include <smf/http/srv/connections.h>
#include <smf/http/srv/session.h>
#include <smf/http/srv/websocket.h>
#include <smf/http/srv/generator.h>

#include <cyng/factory.h>
#include <cyng/object_cast.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/vm/controller.h>
#include <cyng/table/key.hpp>
#include <cyng/table/body.hpp>
#include <cyng/io/serializer.h>

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node 
{
	namespace http
	{
		connections::connections(cyng::logging::log_ptr logger
			, cyng::controller& vm
			, std::string const& doc_root
			, std::string const& nickname
#ifdef NODE_SSL_INSTALLED
			, auth_dirs const& ad
#endif
			, std::map<std::string, std::string> const& redirects
            , std::size_t timeout
			, std::uint64_t max_upload_size
			, bool https_rewrite)
		: logger_(logger)
			, vm_(vm)
			, doc_root_(doc_root)
			, nickname_(nickname)
#ifdef NODE_SSL_INSTALLED
			, auth_dirs_(ad)
#endif
			, redirects_(redirects)
            , timeout_(timeout)
			, max_upload_size_(max_upload_size)
			, https_rewrite_(https_rewrite)
			, uidgen_()
			, sessions_()
			, mutex_()
			, listener_()
		{
			vm_.register_function("modify.web.config", 2, std::bind(&connections::modify_config, this, std::placeholders::_1));
		}

		cyng::controller& connections::vm()
		{
			return vm_;
		}

		void connections::modify_config(cyng::context& ctx)
		{
			//	[http-max-upload-size,888888]
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			auto const name = cyng::value_cast<std::string>(frame.at(0), "");
			if (boost::algorithm::equals(name, "http-max-upload-size")) {
				auto const max_upload_size = cyng::numeric_cast<std::uint64_t>(frame.at(1), 1024 * 1024 * 10);
				CYNG_LOG_INFO(logger_, "http-max-upload-size: " << max_upload_size_ << " => " << max_upload_size);
				max_upload_size_ = max_upload_size;
			}
			else if (boost::algorithm::equals(name, "http-session-timeout")) {
				auto const timeout = cyng::numeric_cast<std::size_t>(frame.at(1), 30u);
				CYNG_LOG_INFO(logger_, "http-session-timeout: " << timeout_.count() << " => " << timeout);
				timeout_ = std::chrono::seconds(timeout);
			}
			else if (boost::algorithm::equals(name, "https-rewrite")) {
				https_rewrite_ = cyng::value_cast(frame.at(1), false);
				CYNG_LOG_INFO(logger_, "https-rewrite is " << (https_rewrite_ ? "on" : "off"));
			}
		}

		bool connections::redirect(std::string& path) const
		{
			if (boost::filesystem::is_directory(path)) {
				path.append("/index.html");
			}

			auto const pos = redirects_.find(path);
			if (pos != redirects_.end()) {

				CYNG_LOG_TRACE(logger_, "redirect " << path << " ==> " << pos->second);
				path = pos->second;
				return true;
			}
			return false;
		}

		void connections::create_session(boost::asio::ip::tcp::socket socket)
		{
			auto const rep = socket.remote_endpoint();

            //
            //  create HTTP session object
            //
            auto obj = cyng::make_object<session>(logger_
				, *this
				, uidgen_()
				, std::move(socket)
                , timeout_
				, max_upload_size_
				, doc_root_
				, nickname_
#ifdef NODE_SSL_INSTALLED
				, auth_dirs_
#endif
				, https_rewrite_);

			auto sp = const_cast<session*>(cyng::object_cast<session>(obj));
			BOOST_ASSERT(sp != nullptr);
			if (sp != nullptr) {

				cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[HTTP_PLAIN]);
				sessions_[HTTP_PLAIN].emplace(sp->tag(), obj);
				sp->run(obj);

				//
				//	populate web session table
				//
				vm_.async_run(node::db::insert("_HTTPSession"
					, cyng::table::key_generator(sp->tag())
					, cyng::table::data_generator(rep, "HTTP", std::chrono::system_clock::now(), false, "?", "initial (HTTP)")
					, 0u
					, vm_.tag()));				
			}
		}

		void connections::upgrade(boost::uuids::uuid tag
			, boost::asio::ip::tcp::socket socket
			, boost::beast::http::request<boost::beast::http::string_body> req)
		{

			//	plain websocket
			auto obj = cyng::make_object<websocket_session>(logger_
				, *this
				, std::move(socket)
				, tag);

			auto wp = const_cast<websocket_session*>(cyng::object_cast<websocket_session>(obj));
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

				wp->do_accept(std::move(req), obj);

				//
				//	update session type
				//
				vm_.async_run(node::db::modify_by_param("_HTTPSession"
					, cyng::table::key_generator(tag)
					, cyng::param_factory("type", "WS")
					, 0u
					, vm_.tag()));

			}
		}

		cyng::object connections::stop_session(boost::uuids::uuid tag)
		{
			//
			//	remove from web session table
			//
			vm_.async_run(node::db::remove("_HTTPSession"
				, cyng::table::key_generator(tag)
				, vm_.tag()));

			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[HTTP_PLAIN]);

			CYNG_LOG_DEBUG(logger_, "connections: remove HTTP session " << tag << " /" << sessions_[HTTP_PLAIN].size());

			auto pos = sessions_[HTTP_PLAIN].find(tag);
			if (pos != sessions_[HTTP_PLAIN].end()) {

				//
				//	get a session reference
				//
				auto obj = pos->second;
				const_cast<session*>(cyng::object_cast<session>(obj))->do_close();
				sessions_[HTTP_PLAIN].erase(pos);
				return obj;
			}

			return cyng::make_object();
		}

		cyng::object connections::stop_ws(boost::uuids::uuid tag)
		{
			vm_.async_run(node::db::remove("_HTTPSession"
				, cyng::table::key_generator(tag)
				, vm_.tag()));

			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[SOCKET_PLAIN]);

			//
			//	Clean up listener list
			//	Remove all listener with the same tag as *wsp.
			//
			for (auto pos = listener_.begin(); pos != listener_.end(); )
			{
				if (cyng::object_cast<websocket_session>(pos->second)->tag() == tag) {
					pos = listener_.erase(pos);
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
				const_cast<websocket_session*>(cyng::object_cast<websocket_session>(obj))->do_close();

				sessions_[SOCKET_PLAIN].erase(pos);
				return obj;
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

				CYNG_LOG_INFO(logger_, "stop " << sessions_[HTTP_PLAIN].size() << " sessions");
				for (auto s : sessions_[HTTP_PLAIN])
				{
					auto ptr = cyng::object_cast<session>(s.second);
					if (ptr)	const_cast<session*>(ptr)->do_close();
				}
			}

			{
				//
				//	unique lock: SOCKET_PLAIN
				//
				cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[SOCKET_PLAIN]);

				CYNG_LOG_INFO(logger_, "stop " << sessions_[SOCKET_PLAIN].size() << " websockets");
	            for (auto & ws : sessions_[SOCKET_PLAIN])
				{
	                auto ptr = cyng::object_cast<websocket_session>(ws.second);
	                if (ptr)	const_cast<websocket_session*>(ptr)->do_shutdown();
				}
			}

			//
			//	clear all maps
			//
			sessions_[HTTP_PLAIN].clear();
			sessions_[SOCKET_PLAIN].clear();
			listener_.clear();

			//
			//	clear web session table
			//
			vm_.async_run(node::db::clear("_HTTPSession"));
		}

		bool connections::ws_msg(boost::uuids::uuid tag, std::string const& msg)
		{
			//
			//	shared lock 
			//
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[SOCKET_PLAIN]);

			auto pos = sessions_[SOCKET_PLAIN].find(tag);
			if (pos != sessions_[SOCKET_PLAIN].end()) {
				auto ptr = cyng::object_cast<websocket_session>(pos->second);
				if (ptr) {
					const_cast<websocket_session*>(ptr)->send_msg(msg);
					return true;
				}
			}
			return false;
		}


		bool connections::add_channel(boost::uuids::uuid tag, std::string const& channel)
		{
			//
			//	unique lock
			//
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[SOCKET_PLAIN]);

			auto pos = sessions_[SOCKET_PLAIN].find(tag);
			if (pos != sessions_[SOCKET_PLAIN].end()) {

				//	test for duplicate entries
				auto const range = listener_.equal_range(channel);
//#ifdef _DEBUG
				for (auto it = range.first; it != range.second; ++it) {
					auto ptr = cyng::object_cast<websocket_session>(it->second);
					if (ptr) {
						if (ptr->tag() == tag) {
							CYNG_LOG_WARNING(logger_, "duplicate entries in channel "
								<< channel
								<< " for SOCKET_PLAIN "
								<< tag);
							listener_.erase(it);
							break;
						}
					}
					else {
						CYNG_LOG_ERROR(logger_, "empty entries in channel "
							<< channel
							<< " for SOCKET_PLAIN");
						listener_.erase(it);
						break;
					}
				}			
//#endif
				//
				//	insert new element
				//
				listener_.emplace(channel, pos->second);
				return true;
			}

			return false;
		}

		void connections::push_event(std::string const& channel, std::string const& msg)
		{
			//
			//	same lock as used for web-socket container
			//	shared lock 
			//
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[SOCKET_PLAIN]);

			auto range = listener_.equal_range(channel);
			for (auto pos = range.first; pos != range.second; )
			{
				auto ptr = cyng::object_cast<websocket_session>(pos->second);
				//  websocket works asynch internally
				if (ptr) {
					const_cast<websocket_session*>(ptr)->send_msg(msg);
					++pos;
				}
				else {
					CYNG_LOG_ERROR(logger_, "websocket_session of channel " << channel << " is NULL");
					pos = listener_.erase(pos);
				}
			}
		}

		bool connections::http_moved(boost::uuids::uuid tag, std::string const& location)
		{
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[HTTP_PLAIN]);
			auto pos = sessions_[HTTP_PLAIN].find(tag);
			if (pos != sessions_[HTTP_PLAIN].end()) {
				auto ptr = cyng::object_cast<session>(pos->second);
				if (ptr != nullptr) {
					const_cast<session*>(ptr)->send_moved(location);
					return true;
				}
			}
			return false;
		}

		bool connections::trigger_download(boost::uuids::uuid tag, boost::filesystem::path const& filename, std::string const& attachment)
		{
			cyng::async::unique_lock<cyng::async::shared_mutex> lock(mutex_[HTTP_PLAIN]);
			auto pos = sessions_[HTTP_PLAIN].find(tag);
			if (pos != sessions_[HTTP_PLAIN].end()) {
				auto ptr = cyng::object_cast<session>(pos->second);
				if (ptr != nullptr) {
					const_cast<session*>(ptr)->trigger_download(filename, attachment);
					return true;
				}
			}
			return false;
		}
	}
}




