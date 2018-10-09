/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/https/srv/connections.h>
#include <smf/https/srv/detector.h>
#include <cyng/object_cast.hpp>

namespace node
{
	namespace https
	{
		connections::connections(cyng::logging::log_ptr logger
			, std::string const& doc_root)
		: logger_(logger)
			, doc_root_(doc_root)
			, uidgen_()
			, sessions_plain_()
			, sessions_ssl_()
		{}

		void connections::create_session(boost::asio::ip::tcp::socket socket
			, boost::asio::ssl::context& ctx)
		{
			std::make_shared<detector>(logger_
				, *this
				, uidgen_()
				, std::move(socket)
				, ctx
				, doc_root_)->run();
		}

		void connections::add_ssl_session(cyng::object obj)
		{
			auto sp = const_cast<ssl_session*>(cyng::object_cast<ssl_session>(obj));
			BOOST_ASSERT(sp != nullptr);
			if (sp != nullptr) {
				sessions_ssl_.emplace(sp->tag(), obj);
				sp->run(obj);
			}
		}

		void connections::add_plain_session(cyng::object obj)
		{
			auto sp = const_cast<plain_session*>(cyng::object_cast<plain_session>(obj));
			BOOST_ASSERT(sp != nullptr);
			if (sp != nullptr) {
				sessions_plain_.emplace(sp->tag(), obj);
				sp->run(obj);
			}
		}
	}
}

