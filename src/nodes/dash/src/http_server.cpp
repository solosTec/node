#include <http_server.h>

#include <smf/http/session.h>
#include <smf/http/ws.h>

#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/parse/json/json_parser.h>
#include <cyng/io/ostream.h>
#include <cyng/sys/memory.h>
#include <cyng/rnd/rnd.hpp>

#include <boost/uuid/uuid_io.hpp>

namespace smf {

	http_server::http_server(boost::asio::io_context& ioc
		, boost::uuids::uuid tag
		, cyng::logger logger
		, std::string const& document_root
		, db& data
		, blocklist_type&& blocklist
		, std::map<std::string, std::string>&& redirects_intrinsic)
	: tag_(tag)
		, logger_(logger)
		, document_root_(document_root)
		, db_(data)
		, blocklist_(blocklist.begin(), blocklist.end())
		, redirects_intrinsic_(redirects_intrinsic.begin(), redirects_intrinsic.end())
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
			redirects_intrinsic_,
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

				if (boost::algorithm::equals(cmd, "exit")) {
					auto const reason = cyng::value_cast(reader["reason"].get(), "uups");
					CYNG_LOG_TRACE(logger_, "[HTTP] ws [" << tag << "] closed: " << reason);

					//
					//	remove from subscription list
					//
					auto const count = db_.remove_from_subscription(tag);
					CYNG_LOG_INFO(logger_, "[HTTP] ws [" << tag << "] removed from " << count << " subscriptions");

					//
					//	remove from ws list
					//
					ws_map_.erase(pos);

					//
					//	iterator invalid!
					//
				}
				else if (boost::algorithm::equals(cmd, "subscribe")) {
					auto const channel = cyng::value_cast(reader["channel"].get(), "uups");
					CYNG_LOG_TRACE(logger_, "[HTTP] ws [" << tag << "] subscribes channel " << channel);

					//
					//	send response
					//
					BOOST_ASSERT(pos->second.lock());
					if (response_subscribe_channel(pos->second.lock(), channel)) {

						//
						//	add to subscription list
						//
						db_.add_to_subscriptions(channel, tag);
					}

				}
				else if (boost::algorithm::equals(cmd, "update")) {
					//	{"cmd":"update","channel":"sys.cpu.usage.total"}
					auto const channel = cyng::value_cast(reader["channel"].get(), "uups");
					CYNG_LOG_TRACE(logger_, "[HTTP] ws [" << tag << "] update channel " << channel);
					response_update_channel(pos->second.lock(), channel);

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


	bool http_server::response_subscribe_channel(ws_sptr wsp, std::string const& name) {


		auto const rel = db_.by_channel(name);
		if (!rel.empty()) {

			BOOST_ASSERT(boost::algorithm::equals(name, rel.channel_));

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
				auto total_size{ tbl->size() };
				std::size_t percent{ 0 }, idx{ 0 };


				tbl->loop([&](cyng::record&& rec, std::size_t idx) -> bool {

					auto const str = json_insert_record(name, rec.to_tuple());
					wsp->push_msg(str);

					//
					//	update current index and percentage calculation
					//	
					++idx;
					const auto prev_percent = percent;
					percent = (100u * idx) / total_size;
					if (prev_percent != percent) {
						wsp->push_msg(json_load_level(name, percent));
					}

					return true;
				});

				//
				//	inform client that data upload is finished
				//
				wsp->push_msg(json_load_icon(name, false));

			}, cyng::access::read(rel.table_));

			return true;	//	valid channel 
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
				return true;	//	valid channel 
			}
#ifdef _DEBUG_DASH
			else if (boost::algorithm::equals(name, "config.bridge")) {
				auto const str1 = json_insert_record(name, 
					cyng::make_tuple(
						cyng::make_param("key", cyng::make_tuple(cyng::make_param("pk", 2))),
						cyng::make_param("data", cyng::make_tuple(cyng::make_param("address", "10.0.1.23"), cyng::make_param("meter", "563412"), cyng::make_param("protocol", "wM-Bus"), cyng::make_param("port", 2000), cyng::make_param("direction", false), cyng::make_param("interval", 60)))
					));
				wsp->push_msg(str1);
				auto const str2 = json_insert_record(name,
					cyng::make_tuple(
						cyng::make_param("key", cyng::make_tuple(cyng::make_param("pk", 4))),
						cyng::make_param("data", cyng::make_tuple(cyng::make_param("address", "10.0.1.24"), cyng::make_param("meter", "563412"), cyng::make_param("port", 6000), cyng::make_param("direction", true), cyng::make_param("interval", 60)))
					));
				wsp->push_msg(str2);

			}
			else if (boost::algorithm::equals(name, "monitor.wMBus")) {
				wsp->push_msg(json_load_icon(name, true));
				auto const str1 = json_insert_record(name,
					cyng::make_tuple(
						cyng::make_param("key", cyng::make_tuple(cyng::make_param("id", 2))),
						cyng::make_param("data", cyng::make_tuple(
							cyng::make_param("ts", "2021-03-03T12:00:01"), 
							cyng::make_param("serverId", "01-e61e-13090016-3c-07"), 
							cyng::make_param("medium", 2), 
							cyng::make_param("manufacturer", "EMH"), 
							cyng::make_param("frameType", 72), 
							cyng::make_param("tag", "tag"),
							cyng::make_param("Payload", "...")))
					));
				wsp->push_msg(str1);
				wsp->push_msg(json_load_icon(name, false));
			}
#endif
			else {
				CYNG_LOG_WARNING(logger_, "[HTTP] subscribe undefined channel " << name);
			}
		}

		return false;
	}

	void http_server::response_update_channel(ws_sptr wsp, std::string const& name) {

		if (boost::algorithm::starts_with(name, "sys.cpu.usage.total"))	{
			auto rnd = cyng::crypto::make_rnd(0, 100);
			wsp->push_msg(json_update_channel(name, rnd()));
		}
		else if (boost::algorithm::starts_with(name, "sys.cpu.count"))	{
			wsp->push_msg(json_update_channel(name, std::thread::hardware_concurrency()));
		}
		else if (boost::algorithm::starts_with(name, "sys.mem.virtual.total"))	{
			wsp->push_msg(json_update_channel(name, cyng::sys::get_total_ram()));
		}
		else if (boost::algorithm::starts_with(name, "sys.mem.virtual.used"))	{
			wsp->push_msg(json_update_channel(name, cyng::sys::get_used_ram()));
		}
		else if (boost::algorithm::starts_with(name, "sys.mem.virtual.stat"))	{
			wsp->push_msg(json_update_channel(name, 402));
		}
		else
		{
			CYNG_LOG_WARNING(logger_, "[HTTP] update undefined channel " << name);
		}
	}

	void http_server::notify_remove(std::string channel, cyng::key_t key, boost::uuids::uuid tag) {
		auto const pos = ws_map_.find(tag);
		if (pos != ws_map_.end()) {
			auto sp = pos->second.lock();
			if (sp) sp->push_msg(json_delete_record(channel, key));
			else {
				//	shouldn't be necessary
				BOOST_ASSERT_MSG(false, "invalid ws map - notify_remove");
				ws_map_.erase(pos);
			}
		}
	}

	void http_server::notify_clear(std::string channel, boost::uuids::uuid tag) {
		auto const pos = ws_map_.find(tag);
		if (pos != ws_map_.end()) {
			auto sp = pos->second.lock();
			if (sp) sp->push_msg(json_clear_table(channel));
			else {
				//	shouldn't be necessary
				BOOST_ASSERT_MSG(false, "invalid ws map - notify_clear");
				ws_map_.erase(pos);
			}
		}
	}

	void http_server::notify_insert(std::string channel, cyng::record&& rec, boost::uuids::uuid tag) {
		auto const pos = ws_map_.find(tag);
		if (pos != ws_map_.end()) {
			auto sp = pos->second.lock();
			if (sp) sp->push_msg(json_insert_record(channel, rec.to_tuple()));
			else {
				//	shouldn't be necessary
				BOOST_ASSERT_MSG(false, "invalid ws map - notify_insert");
				ws_map_.erase(pos);
			}
		}

	}

	std::string json_insert_record(std::string channel, cyng::tuple_t&& tpl) {

		return cyng::io::to_json(cyng::make_tuple(
			cyng::make_param("cmd", "insert"),
			cyng::make_param("channel", channel),
			cyng::make_param("rec", std::move(tpl))));

	}

	std::string json_update_record(std::string channel, cyng::tuple_t&& tpl) {

		return cyng::io::to_json(cyng::make_tuple(
			cyng::make_param("cmd", "update"),
			cyng::make_param("channel", channel),
			cyng::make_param("rec", std::move(tpl))));

	}

	std::string json_load_icon(std::string channel, bool b) {
		return cyng::io::to_json(cyng::make_tuple(
			cyng::make_param("cmd", "load"),
			cyng::make_param("channel", channel),
			cyng::make_param("show", b)));

	}

	std::string json_load_level(std::string channel, std::size_t level) {

		return cyng::io::to_json(cyng::make_tuple(
			cyng::make_param("cmd", "load"),
			cyng::make_param("channel", channel),
			cyng::make_param("level", level)));

	}


	std::string json_delete_record(std::string channel, cyng::key_t const& key) {

		return cyng::io::to_json(cyng::make_tuple(
			cyng::make_param("cmd", "delete"),
			cyng::make_param("channel", channel),
			cyng::make_param("key", key)));

	}

	std::string json_clear_table(std::string channel) {

		return cyng::io::to_json(cyng::make_tuple(
			cyng::make_param("cmd", "clear"),
			cyng::make_param("channel", channel)));

	}

}


