#include <http_server.h>

#include <smf/http/session.h>
#include <smf/http/ws.h>

#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/parse/json/json_parser.h>
#include <cyng/io/ostream.h>

//#include <iostream>

#include <boost/uuid/uuid_io.hpp>

namespace smf {

	http_server::http_server(boost::asio::io_context& ioc
		, boost::uuids::uuid tag
		, cyng::logger logger
		, std::string const& document_root
		, db& data
		, blocklist_type&& blocklist)
	: tag_(tag)
		, logger_(logger)
		, document_root_(document_root)
		, db_(data)
		, blocklist_(blocklist.begin(), blocklist.end())
		, server_(ioc, logger, std::bind(&http_server::accept, this, std::placeholders::_1))
		, uidgen_()
		, ws_map_()
	{
		CYNG_LOG_INFO(logger_, "[HTTP] server " << tag << " started");
	}

	http_server::~http_server()
	{
#ifdef _DEBUG_DASH
		std::cout << "http_server(~)" << std::endl;
#endif
	}


	void http_server::stop(cyng::eod)
	{
		CYNG_LOG_WARNING(logger_, "stop http server task(" << tag_ << ")");
		server_.stop();
	}

	void http_server::listen(boost::asio::ip::tcp::endpoint ep) {

		CYNG_LOG_INFO(logger_, "start listening at(" << ep << ")");
		server_.start(ep);

	}

	void http_server::accept(boost::asio::ip::tcp::socket&& s) {

		CYNG_LOG_INFO(logger_, "accept (" << s.remote_endpoint() << ")");

		//
		//	check blocklist
		//
		auto const pos = blocklist_.find(s.remote_endpoint().address());
		if (pos != blocklist_.end()) {

			CYNG_LOG_WARNING(logger_, "address ["
				<< s.remote_endpoint()
				<< "] is blocked");
			s.close();

			return;
		}

		std::make_shared<http::session>(
			std::move(s),
			logger_,
			document_root_,
			db_.cfg_.get_value("http-max-upload-size", static_cast<std::uint64_t>(0xA00000)),
			db_.cfg_.get_value("http-server-nickname", "coraline"),
			db_.cfg_.get_value("http-session-timeout", std::chrono::seconds(30)),
			std::bind(&http_server::upgrade, this, std::placeholders::_1, std::placeholders::_2))->run();
	}

	void http_server::upgrade(boost::asio::ip::tcp::socket s
		, boost::beast::http::request<boost::beast::http::string_body> req) {

		/*boost::asio::make_strand(server_.ioc_);*/
		auto const tag = uidgen_();
		CYNG_LOG_INFO(logger_, "new ws " << tag << '@' << s.remote_endpoint() << " #" << ws_map_.size() + 1);
		ws_map_.emplace(tag, std::make_shared<http::ws>(std::move(s)
			, server_.ioc_
			, logger_
			, std::bind(&http_server::on_msg, this, tag, std::placeholders::_1))).first->second.lock()->do_accept(std::move(req));
	}

	void http_server::on_msg(boost::uuids::uuid tag, std::string msg) {

		//
		//	JSON parser
		//
		CYNG_LOG_TRACE(logger_, "ws (" << tag << ") received " << msg);
		cyng::json::parser parser([this, tag](cyng::object&& obj) {

			CYNG_LOG_TRACE(logger_, "[HTTP] ws parsed " << obj);
			auto const reader = cyng::make_reader(std::move(obj));
			auto const cmd = cyng::value_cast(reader["cmd"].get(), "uups");

			auto pos = ws_map_.find(tag);
			if (pos != ws_map_.end() && !pos->second.expired()) {

				if (boost::algorithm::equals(cmd, "subscribe")) {
					auto const channel = cyng::value_cast(reader["channel"].get(), "uups");
					CYNG_LOG_TRACE(logger_, "[HTTP] ws [" << tag << "] subscribes channel " << channel);
					response_subscribe_channel(pos->second.lock(), channel);

				}
				else {
					CYNG_LOG_WARNING(logger_, "[HTTP] unknown ws command " << cmd);
				}
			}
			else {

				CYNG_LOG_WARNING(logger_, "[HTTP] ws [" << tag << "] is not registered");

			}

		});

		parser.read(msg.begin(), msg.end());
	}

	void http_server::response_subscribe_channel(ws_sptr wsp, std::string const& name) {


		auto const rel = db_.by_channel(name);
		if (!rel.empty()) {

			BOOST_ASSERT(boost::algorithm::equals(name, rel.channel_));

			//
			//	ToDo: add subscription table
			//
			
			// 
			//	Send initial data set
			//
			db_.cache_.access([&](cyng::table const* tbl) -> void {

				//
				//	inform client that data upload is starting
				//
				wsp->push_msg(json_load_icon(name, true));

				//
				//	get total record size
				//
				//auto size{ tbl->size() };
				//std::size_t percent{ 0 }, idx{ 0 };


				tbl->loop([&](cyng::record&& rec, std::size_t idx) -> bool {

					auto const str = json_insert_record(name, rec.to_tuple());
					wsp->push_msg(str);

					return true;
				});

				//
				//	inform client that data upload is finished
				//
				wsp->push_msg(json_load_icon(name, false));

			}, cyng::access::read(rel.table_));
		}
		else {

			auto const rel = db_.by_counter(name);
			if (!rel.empty()) {

				//
				//	ToDo: add subscription table
				//

				//
				//	Send table size
				//
				auto const str =  json_update_channel(name, db_.cache_.size(rel.table_));
				wsp->push_msg(str);

			}
			else {
				CYNG_LOG_WARNING(logger_, "[HTTP] undefined channel " << name);
			}
		}
	}

	std::string json_insert_record(std::string channel, cyng::tuple_t&& tpl) {

		return cyng::io::to_json(cyng::make_tuple(
			cyng::make_param("cmd", "insert"),
			cyng::make_param("channel", channel),
			cyng::make_param("rec", std::move(tpl))));

	}

	std::string json_load_icon(std::string channel, bool b) {
		return cyng::io::to_json(cyng::make_tuple(
			cyng::make_param("cmd", "load"),
			cyng::make_param("channel", channel),
			cyng::make_param("show", b)));

	}

}


