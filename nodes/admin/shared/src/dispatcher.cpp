/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "dispatcher.h"
#include "tables.h"
#include <cyng/json.h>
#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/sys/memory.h>
#include <cyng/json/json_parser.h>

#include <boost/algorithm/string.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node 
{

	dispatcher::dispatcher(cyng::logging::log_ptr logger, connection_manager_interface& cm)
		: logger_(logger)
		, connection_manager_(cm)
	{}

	void dispatcher::register_this(cyng::controller& vm)
	{
		vm.register_function("store.relation", 2, std::bind(&dispatcher::store_relation, this, std::placeholders::_1));
		vm.register_function("bus.res.proxy.gateway", 9, std::bind(&dispatcher::res_proxy_gateway, this, std::placeholders::_1));
		vm.register_function("bus.res.proxy.job", 9, std::bind(&dispatcher::res_proxy_job, this, std::placeholders::_1));
		vm.register_function("bus.res.attention.code", 7, std::bind(&dispatcher::res_attention_code, this, std::placeholders::_1));

		vm.register_function("http.move", 2, std::bind(&dispatcher::http_move, this, std::placeholders::_1));
	}

	void dispatcher::store_relation(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));
		//	cluster seq => ws tag
	}

	void dispatcher::res_proxy_gateway(cyng::context& ctx)
	{
		//	[a7114557-b0f4-4269-82be-5c46f1e9f75b,9f773865-e4af-489a-8824-8f78a2311278,6,[f72f7307-40e6-483b-8106-115290f8f1fe],2697aa30-ec69-4766-9e4d-b312c7b29c25,get.proc.param,01-e61e-29436587-bf-03,root-data-prop,%(("active":true),("idx":2),("period":00000000),("profile":null),("registers":[8181C78203FF,0700030000FF,0000616100FF,0000600101FF,0000600100FF,0000616100FF]),("size":00000064))]
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] ident
			boost::uuids::uuid,		//	[1] source
			std::uint64_t,			//	[2] sequence
			cyng::vector_t,			//	[3] gw key
			boost::uuids::uuid,		//	[4] websocket tag
			std::string,			//	[5] channel
			std::string,			//	[6] server id
			cyng::vector_t,			//	[7] section / "OBIS path"
			cyng::param_map_t		//	[8] params
		>(frame);


		//	[3bb02dd1-b864-474b-b131-7ab85f3862e9,9f773865-e4af-489a-8824-8f78a2311278,19,[8d04b8e0-0faf-44ea-b32b-8405d407f2c1],ea7a2ee6-56ae-4536-9600-45c8dd2c2e9e,get.list.request,,list-current-data-record,%(("08 00 01 00 00 ff":0.758),("08 00 01 02 00 ff":0.758))]
		//	{"cmd": "update", "channel": "get.list.request", "section": "list-current-data-record", "rec": {"srv": "", "values": {"08 00 01 00 00 ff":"0.758","08 00 01 02 00 ff":"0.758"}}}
		auto data = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", std::get<5>(tpl)),
			cyng::param_factory("section", std::get<7>(tpl)),
			cyng::param_factory("gw", std::get<3>(tpl)),
			cyng::param_factory("rec", cyng::tuple_factory(
				cyng::param_factory("srv", std::get<6>(tpl)),
				cyng::param_factory("values", std::get<8>(tpl))
			)));

		//	{"cmd": "update", "channel": "status.gateway.word", "rec": {"srv": "00:ff:b0:0b:ca:ae", "word": {"256":false,"8192":true,"16384":false,"65536":true,"131072":true,"262144":true,"524288":false,"4294967296":false}}}
		auto msg = cyng::json::to_string(data);
#ifdef _DEBUG
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " JSON: " << msg);
#endif
		connection_manager_.ws_msg(std::get<4>(tpl), msg);
	}

	void dispatcher::res_proxy_job(cyng::context& ctx)
	{
		//	[71dca95e-7765-4ecf-af8b-078c42ec8afe,9f773865-e4af-489a-8824-8f78a2311278,2,[2c0fc607-95d6-4843-a8d5-fec087780447],8db233d3-2652-4ee5-856c-bfcb1ee640ef,cache.sections,0500153B01EC46,[810060050000,81490D0700FF,81811006FFFF,81811106FFFF,81818160FFFF],%()]
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));
		auto tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] ident
			boost::uuids::uuid,		//	[1] source
			std::uint64_t,			//	[2] sequence
			cyng::vector_t,			//	[3] gw key
			boost::uuids::uuid,		//	[4] websocket tag (origin)
			std::string,			//	[5] channel (message type)
			std::string,			//	[6] server id
			cyng::vector_t,			//	[7] vector of root paths
			cyng::param_map_t		//	[8] params
		>(frame);

		auto data = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", std::get<5>(tpl)),
			cyng::param_factory("section", std::get<7>(tpl)),
			cyng::param_factory("rec", cyng::tuple_factory(
				cyng::param_factory("srv", std::get<6>(tpl)),
				cyng::param_factory("gw", std::get<3>(tpl).at(0)),
				cyng::param_factory("values", std::get<8>(tpl))
			)));

		//	{"cmd": "update", "channel": "cache.sections", "section": ["810060050000", "81490D0700FF", "81811006FFFF", "81811106FFFF", "81818160FFFF"], "rec": {"srv": "0500153b01ec46", "gw": "2c0fc607-95d6-4843-a8d5-fec087780447", "values": {}}}
		auto msg = cyng::json::to_string(data);
#ifdef _DEBUG
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " JSON: " << msg);
#endif
		connection_manager_.ws_msg(std::get<4>(tpl), msg);

	}

	void dispatcher::res_attention_code(cyng::context& ctx)
	{
		//	[da673931-9743-41b9-8a46-6ce946c9fa6c,9f773865-e4af-489a-8824-8f78a2311278,4,5c200bdf-22c0-41bd-bc93-d879d935889e,00:15:3b:02:29:81,8181C7C7FE03,NO SERVER ID]
		//
		//	* [uuid] ident
		//	* [uuid] source
		//	* [u64] cluster seq
		//	* [uuid] ws tag
		//	* [string] server id
		//	* [buffer] attention code
		//	* [string] message
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] ident
			boost::uuids::uuid,		//	[1] source
			std::uint64_t,			//	[2] sequence
			boost::uuids::uuid,		//	[3] ws tag
			std::string,			//	[4] server id
			cyng::buffer_t,			//	[5] attention code (OBIS)
			std::string				//	[6] msg
		>(frame);

		auto data = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", "attention.code"),
			cyng::param_factory("section", std::get<5>(tpl)),
			cyng::param_factory("rec", cyng::tuple_factory(
				cyng::param_factory("srv", std::get<4>(tpl)),
				cyng::param_factory("values", std::get<6>(tpl))
			)));

		//	{"cmd": "update", "channel": "status.gateway.word", "rec": {"srv": "00:ff:b0:0b:ca:ae", "word": {"256":false,"8192":true,"16384":false,"65536":true,"131072":true,"262144":true,"524288":false,"4294967296":false}}}
		auto msg = cyng::json::to_string(data);
		connection_manager_.ws_msg(std::get<3>(tpl), msg);

	}

	void dispatcher::subscribe(cyng::store::db& db)
	{
		for (auto const& rel : channel::rel_) {

			CYNG_LOG_INFO(logger_, "install DB slot " << rel.table_);
			auto tmp = db.get_listener(rel.table_
				, std::bind(&dispatcher::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
				, std::bind(&dispatcher::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
				, std::bind(&dispatcher::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
				, std::bind(&dispatcher::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

			boost::ignore_unused(tmp);
		}

		CYNG_LOG_INFO(logger_, "db has " << db.num_all_slots() << " connected slots");
	}

	void dispatcher::sig_insert(cyng::table::record const& rec, std::string channel)
	{
		auto const tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("insert")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("rec", rec.convert()));

		auto msg = cyng::json::to_string(tpl);
		connection_manager_.push_event(channel, msg);
	}

	void dispatcher::sig_ins(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::table::data_type const& data
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		cyng::table::record rec(tbl->meta_ptr(), key, data, gen);

		auto const rel = channel::find_rel_by_table(tbl->meta().get_name());
		if (!rel.empty()) {

			sig_insert(rec, rel.channel_);
			if (rel.has_counter()) {
				update_channel(rel.counter_, tbl->size());
			}
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "sig.ins - unknown table "
				<< tbl->meta().get_name());
		}
	}

	void dispatcher::sig_delete(cyng::table::key_type const& key, std::string channel)
	{
		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("delete")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("key", key));

		auto msg = cyng::json::to_string(tpl);
		connection_manager_.push_event(channel, msg);
	}

	void dispatcher::sig_del(cyng::store::table const* tbl, cyng::table::key_type const& key, boost::uuids::uuid source)
	{
		auto const rel = channel::find_rel_by_table(tbl->meta().get_name());
		if (!rel.empty()) {

			sig_delete(key, rel.channel_);
			if (rel.has_counter()) {
				update_channel(rel.counter_, tbl->size());
			}
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "sig.del - unknown table "
				<< tbl->meta().get_name());
		}
	}

	void dispatcher::sig_clear(std::string channel)
	{
		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("clear")),
			cyng::param_factory("channel", channel));

		auto msg = cyng::json::to_string(tpl);
		connection_manager_.push_event(channel, msg);
	}

	void dispatcher::sig_clr(cyng::store::table const* tbl, boost::uuids::uuid source)
	{
		auto const rel = channel::find_rel_by_table(tbl->meta().get_name());
		if (!rel.empty()) {

			sig_clear(rel.channel_);
			if (rel.has_counter()) {
				update_channel(rel.counter_, tbl->size());
			}
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "sig.clr - unknown table "
				<< tbl->meta().get_name());
		}
	}

	void dispatcher::sig_mod(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::attr_t const& attr
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		//	to much noise
		//CYNG_LOG_DEBUG(logger_, "sig.mod - "
		//	<< tbl->meta().get_name());

#ifdef _DEBUG
		//if (boost::algorithm::equals(tbl->meta().get_name(), "_Config")) {

		//	CYNG_LOG_DEBUG(logger_, "sig.mod - "
		//		<< cyng::io::to_str(attr.second));

		//}
#endif

		auto const rel = channel::find_rel_by_table(tbl->meta().get_name());
		if (!rel.empty()) {

			//
			//	convert attribute to parameter (as map)
			//
			auto f_val = [&]()->cyng::object {

				auto pm = tbl->meta().to_param_map(attr);

				if (boost::algorithm::equals(tbl->meta().get_name(), "_Config")) {

					return (pm.empty())
						? cyng::make_object()
						: pm.begin()->second
						;
				}
				return cyng::make_object(std::move(pm));
			};

			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", rel.channel_),
				cyng::param_factory("key", key),
				cyng::param_t("value", f_val()));

			auto const msg = cyng::json::to_string(tpl);
			connection_manager_.push_event(rel.channel_, msg);

		}
		else
		{
			CYNG_LOG_ERROR(logger_, "sig.mod - unknown table "
				<< tbl->meta().get_name());
		}
	}

	void dispatcher::update_channel(std::string const& channel, std::size_t size)
	{
		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("value", size));

		auto const msg = cyng::json::to_string(tpl);
		connection_manager_.push_event(channel, msg);
	}

	void dispatcher::subscribe_channel(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		auto const rel = channel::find_rel_by_channel(channel);
		if (!rel.empty())	{
			subscribe(db, rel.table_, channel, tag);
		}
		else {
			auto const rel = channel::find_rel_by_counter(channel);
			if (!rel.empty()) 
			{
				subscribe_table_count(db, channel, rel.table_, tag);
			}
			else if (boost::algorithm::starts_with(channel, "config.web"))
			{
				subscribe_web(db, channel, tag);
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "ws.read - unknown subscribe channel [" << channel << "]");
			}
		}
	}

	void dispatcher::pull(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		if (boost::algorithm::starts_with(channel, "sys.cpu.usage.total"))
		{
			update_sys_cpu_usage_total(db, channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "sys.cpu.count"))
		{
			update_sys_cpu_count(channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "sys.mem.virtual.total"))
		{
			update_sys_mem_virtual_total(channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "sys.mem.virtual.used"))
		{
			update_sys_mem_virtual_used(channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "sys.mem.virtual.stat"))
		{
			update_sys_mem_virtual_stat(channel, tag);
		}
		else
		{
			CYNG_LOG_WARNING(logger_, "ws.read - unknown update channel [" << channel << "]");
		}
	}

	void dispatcher::query_channel(cyng::store::db& cache, std::string const& channel, boost::uuids::uuid tag, cyng::vector_t const& key)
	{
		try {
			auto const rel = channel::find_rel_by_channel(channel);

			auto const rec = cache.lookup(rel.table_, key);
			if (!rec.empty()) {

				auto const row = rec.convert();

				CYNG_LOG_DEBUG(logger_, "ws.read - query channel ["
					<< channel
					<< "] "
					<< cyng::io::to_type(row));

				auto const tpl = cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("update")),
					cyng::param_factory("channel", channel),
					cyng::param_factory("rec", rec.convert()));

				auto const msg = cyng::json::to_string(tpl);
				connection_manager_.ws_msg(tag, msg);

			}
			else {
				CYNG_LOG_WARNING(logger_, "ws.read - query channel ["
					<< channel
					<< "] has no data");
			}

		}
		catch (std::exception const& ex) {
			CYNG_LOG_ERROR(logger_, "ws.read - query channel ["
				<< channel <<
				"] failed:"
				<< ex.what());
		}
	}

	void dispatcher::update_sys_cpu_usage_total(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		db.access([&](cyng::store::table* tbl) {
			const auto rec = tbl->lookup(cyng::table::key_generator("cpu:load"));
			if (!rec.empty())
			{

				CYNG_LOG_TRACE(logger_, "sys.cpu.usage.total: " << cyng::io::to_str(rec["value"]));

				//
				//	Total CPU load of the system in %
				//
				auto tpl = cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("update")),
					cyng::param_factory("channel", channel),
					cyng::param_t("value", rec["value"]));

				auto msg = cyng::json::to_string(tpl);
				connection_manager_.ws_msg(tag, msg);
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "record cpu:load not found");
			}
		}, cyng::store::write_access("_Config"));
	}

	void dispatcher::update_sys_cpu_count(std::string const& channel, boost::uuids::uuid tag)
	{
		//
		//	CPU count
		//
		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("value", std::thread::hardware_concurrency()));

		auto const msg = cyng::json::to_string(tpl);

		connection_manager_.ws_msg(tag, msg);

	}

	void dispatcher::update_sys_mem_virtual_total(std::string const& channel, boost::uuids::uuid tag)
	{
		//
		//	total virtual memory in bytes
		//
		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("value", cyng::sys::get_total_virtual_memory()));

		auto const msg = cyng::json::to_string(tpl);

		connection_manager_.ws_msg(tag, msg);
	}

	void dispatcher::update_sys_mem_virtual_used(std::string const& channel, boost::uuids::uuid tag)
	{
		//
		//	used virtual memory in bytes
		//
		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("value", cyng::sys::get_used_virtual_memory()));

		auto const msg = cyng::json::to_string(tpl);

		connection_manager_.ws_msg(tag, msg);
	}

	void dispatcher::update_sys_mem_virtual_stat(std::string const& channel, boost::uuids::uuid tag)
	{
		//
		//	used virtual memory in bytes
		//
		const auto used = cyng::sys::get_used_virtual_memory();
		const auto total = cyng::sys::get_total_virtual_memory();

		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("value", cyng::param_map_factory("total", total)("used", used)("percent", (used * 100.0) / total)()));

		auto const msg = cyng::json::to_string(tpl);

		connection_manager_.ws_msg(tag, msg);
	}

	void dispatcher::subscribe_web(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		//
		//	install channel
		//
		connection_manager_.add_channel(tag, channel);

		//
		//	send initial data set
		//
		subscribe_web(db, channel, tag, "https-available");
		subscribe_web(db, channel, tag, "https-rewrite");
		subscribe_web(db, channel, tag, "http-max-upload-size");
		subscribe_web(db, channel, tag, "http-session-timeout");
		subscribe_web(db, channel, tag, "https-max-upload-size");
		subscribe_web(db, channel, tag, "https-session-timeout");
	}

	void dispatcher::subscribe_web(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag, std::string name)
	{
		auto const obj = db.get_value("_Config", "value", name);
		//BOOST_ASSERT_MSG(!obj.is_null(), "name not found");

		auto const tpl = cyng::tuple_factory(cyng::param_factory("cmd", "insert")
			, cyng::param_factory("channel", channel)
			, cyng::param_factory("rec", cyng::tuple_factory(cyng::param_factory("key", cyng::param_factory("name", name))
				, cyng::param_factory("data", cyng::param_factory("value", obj)))));

		auto const msg = cyng::json::to_string(tpl);
		connection_manager_.ws_msg(tag, msg);
	}

	void dispatcher::subscribe(cyng::store::db& db, std::string table, std::string const& channel, boost::uuids::uuid tag)
	{
		//
		//	install channel
		//
		connection_manager_.add_channel(tag, channel);

		if (boost::algorithm::equals(table, "TGUIUser")) {
			subscribe_TGUIUser(db, channel, tag);
		}
		else {
			subscribe_with_loop(db, table, channel, tag);
		}
	}

	void dispatcher::subscribe_TGUIUser(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		//
		//	send initial data set of TGUIUser table
		//
		db.access([&](cyng::store::table const* tbl_hs, cyng::store::table const* tbl_user) {

			auto rec_hs = tbl_hs->lookup(cyng::table::key_generator(tag));
			if (!rec_hs.empty()) {

				//
				//	session found
				//	lookup GUI user by name
				//
				auto const user_name = cyng::value_cast<std::string>(rec_hs["user"], "?");
				CYNG_LOG_DEBUG(logger_, "(TGUIUser)lookup user " << user_name);
				auto rec_user = tbl_user->lookup_by_index(rec_hs["user"]);
				if (!rec_user.empty()) {

					auto privs = cyng::value_cast<std::string>(rec_user["privs"], "{}");
					CYNG_LOG_TRACE(logger_, "TGUIUser: " << privs);
					try {
						auto const r = cyng::parse_json(privs);
						if (r.second) {

							//cyng::table::record tmp(rec);
							rec_user.set("privs", r.first);

							auto tpl = cyng::tuple_factory(
								cyng::param_factory("cmd", std::string("insert")),
								cyng::param_factory("channel", channel),
								cyng::param_factory("rec", rec_user.convert()));

							auto msg = cyng::json::to_string(tpl);
							connection_manager_.ws_msg(tag, msg);
						}
						else {
							CYNG_LOG_WARNING(logger_, "invalid privs: " << privs);
						}
					}
					catch (std::exception const& ex) {
						CYNG_LOG_ERROR(logger_, ex.what()
							<< " - converting TGUIUser to JSON failed ");

					}
				}
				else {

					CYNG_LOG_ERROR(logger_, "(TGUIUser) user " << user_name << " not found");
					if (boost::algorithm::equals(user_name, "?")) {

						//
						//	The server config file contains no "auth" section.
						//	Deliver some bogus values.
						//
					}
				}
			}
			else {

				CYNG_LOG_ERROR(logger_, "(_HTTPSession) user of session " << tag << " not found");

			}

		}	, cyng::store::read_access("_HTTPSession")
			, cyng::store::read_access("TGUIUser"));

	}

	void dispatcher::subscribe_with_loop(cyng::store::db& db
		, std::string table
		, std::string const& channel
		, boost::uuids::uuid tag)
	{
		//
		//	send initial data set of device table
		//
		db.access([&](cyng::store::table const* tbl) {

			//
			//	inform client that data upload is starting
			//
			display_loading_icon(tag, true, channel);

			//
			//	get total record size
			//
			auto size{ tbl->size() };
			std::size_t percent{ 0 }, idx{ 0 };

			//
			//	upload data
			//
			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				CYNG_LOG_TRACE(logger_, "ws.read - insert " << table << cyng::io::to_str(rec.key()));

				auto tpl = cyng::tuple_factory(
					cyng::param_factory("cmd", std::string("insert")),
					cyng::param_factory("channel", channel),
					cyng::param_factory("rec", rec.convert()));

				//
				//	convert to JSON may fail 
				//
				try {
					auto msg = cyng::json::to_string(tpl);
					connection_manager_.ws_msg(tag, msg);

					++idx;

					//
					//	calculate charge status in percent
					//
					const auto prev_percent = percent;
					percent = (100u * idx) / size;

					//
					//	don't send fractions
					//
					if (prev_percent != percent) {
						display_loading_level(tag, percent, channel);
					}
				}
				catch (std::exception const& ex) {
					CYNG_LOG_ERROR(logger_, ex.what()
						<< " - converting "
						<< table
						<< " to JSON failed "
						<< cyng::io::to_str(tpl));

				}

				return true;	//	continue
			});

			BOOST_ASSERT(counter == 0);
			boost::ignore_unused(counter);	//	release version
			CYNG_LOG_INFO(logger_, "channel " 
				<< channel
				<< " sent "
				<< tbl->size() 
				<< ' ' 
				<< tbl->meta().get_name() 
				<< " records to client "
				<< tag);

			//
			//	make sure 100% is complete
			//
			if (percent < 100) {
				percent = 100;
				display_loading_level(tag, percent, channel);
			}

			//
			//	inform client that data upload is finished
			//
			display_loading_icon(tag, false, channel);

		}, cyng::store::read_access(table));
	}

	void dispatcher::subscribe_table_count(cyng::store::db& db, std::string const& channel, std::string const& table, boost::uuids::uuid tag)
	{
		connection_manager_.add_channel(tag, channel);
		auto const size = db.size(table);
		update_channel(channel, size);

	}

	void dispatcher::display_loading_icon(boost::uuids::uuid tag, bool b, std::string const& channel)
	{
		connection_manager_.ws_msg(tag
			, cyng::json::to_string(cyng::tuple_factory(cyng::param_factory("cmd", std::string("load"))
				, cyng::param_factory("channel", channel)
				, cyng::param_factory("show", b))));
	}

	void dispatcher::display_loading_level(boost::uuids::uuid tag, std::size_t level, std::string const& channel)
	{
		connection_manager_.ws_msg(tag
			, cyng::json::to_string(cyng::tuple_factory(cyng::param_factory("cmd", std::string("load"))
				, cyng::param_factory("channel", channel)
				, cyng::param_factory("level", level))));
	}

	void dispatcher::http_move(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] source
			std::string			//	[1] target
		>(frame);

		//	send 302 - Object moved response
		connection_manager_.http_moved(std::get<0>(tpl), std::get<1>(tpl));
	}

}
