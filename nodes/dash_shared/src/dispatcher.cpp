/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "dispatcher.h"
#include <cyng/json.h>
#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/sys/memory.h>

#include <boost/algorithm/string.hpp>
#include <boost/core/ignore_unused.hpp>

namespace node 
{
	dispatcher::dispatcher(cyng::logging::log_ptr logger, connection_manager_interface& cm)
		: logger_(logger)
		, connection_manager_(cm)
	{}

	void dispatcher::register_this(cyng::controller& vm)
	{
		vm.register_function("store.relation", 2, std::bind(&dispatcher::store_relation, this, std::placeholders::_1));

		vm.register_function("bus.res.query.status.word", 5, std::bind(&dispatcher::res_query_status_word, this, std::placeholders::_1));
		vm.register_function("bus.res.query.srv.visible", 9, std::bind(&dispatcher::res_query_srv_visible, this, std::placeholders::_1));
		vm.register_function("bus.res.query.srv.active", 9, std::bind(&dispatcher::res_query_srv_active, this, std::placeholders::_1));
		vm.register_function("bus.res.query.firmware", 8, std::bind(&dispatcher::res_query_firmware, this, std::placeholders::_1));
		vm.register_function("bus.res.query.memory", 6, std::bind(&dispatcher::res_query_memory, this, std::placeholders::_1));

		vm.register_function("http.move", 2, std::bind(&dispatcher::http_move, this, std::placeholders::_1));

	}

	void dispatcher::store_relation(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "store.relation - " << cyng::io::to_str(frame));
		//	cluster seq => ws tag
	}

	void dispatcher::res_query_status_word(cyng::context& ctx)
	{
		//	 [3cb44588-3075-4086-b684-57a4bab6e26c,2,a5d83e14-dc3e-4105-95d0-034c3b69b991,00:ff:b0:0b:ca:ae,cc070202]
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "bus.res.query.status.word - " << cyng::io::to_str(frame));

		auto const data = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] source
			std::uint64_t,			//	[1] sequence
			boost::uuids::uuid,		//	[2] websocket tag
			std::string,			//	[3] server id (key)
			cyng::attr_map_t		//	[4] status word
		>(frame);

		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", "status.gateway.word"),
			cyng::param_factory("rec", cyng::tuple_factory(
				cyng::param_factory("srv", std::get<3>(data)),
				cyng::param_factory("word", std::get<4>(data))
			)));

		//	{"cmd": "update", "channel": "status.gateway.word", "rec": {"srv": "00:ff:b0:0b:ca:ae", "word": {"256":false,"8192":true,"16384":false,"65536":true,"131072":true,"262144":true,"524288":false,"4294967296":false}}}
		auto msg = cyng::json::to_string(tpl);
		//CYNG_LOG_TRACE(logger_, "bus.res.query.status.word - " << msg);
		connection_manager_.ws_msg(std::get<2>(data), msg);

	}

	void dispatcher::res_query_srv_visible(cyng::context& ctx)
	{
		//	[d6529e83-fa7f-469d-8681-798501842252,1,637c19e7-b20d-462f-afbf-81fdbb064c82,0005,00:15:3b:02:29:7e,01-e61e-29436587-bf-03,---,2018-08-30 09:37:13.00000000]
		//	[ea0a8415-65a1-47d0-8572-3f417cb6b408,3,b7ee88d9-e839-4ee7-bbe4-ca12f2ecf246,0003,00:15:3b:02:29:7e,01-e61e-13090016-3c-07,---,2018-10-15 14:41:11.00000000,00000000]
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "bus.res.query.srv.visible - " << cyng::io::to_str(frame));

		auto const data = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] source
			std::uint64_t,			//	[1] sequence
			boost::uuids::uuid,		//	[2] websocket tag
			std::uint16_t,			//	[3] list number
			std::string,			//	[4] server id (key)
			std::string,			//	[5] meter
			std::string,			//	[6] device class
			std::chrono::system_clock::time_point,	//	[7] last status update
			std::uint32_t			//	[8] server type
		>(frame);

		//
		//	reformatting timestamp
		//
		auto stp = (cyng::chrono::same_day(std::get<7>(data), std::chrono::system_clock::now()))
			? cyng::ts_to_str(cyng::chrono::duration_of_day(std::get<7>(data)))
			: cyng::date_to_str(std::get<7>(data))
			;


		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("append")),
			cyng::param_factory("channel", "status.gateway.visible"),
			cyng::param_factory("rec", cyng::tuple_factory(
				cyng::param_factory("nr", std::get<3>(data)),
				cyng::param_factory("srv", std::get<4>(data)),
				cyng::param_factory("meter", std::get<5>(data)),
				cyng::param_factory("class", std::get<6>(data)),
				cyng::param_factory("tp", stp),
				cyng::param_factory("type", std::get<8>(data))
			)));

		auto msg = cyng::json::to_string(tpl);
		connection_manager_.ws_msg(std::get<2>(data), msg);

	}

	void dispatcher::res_query_srv_active(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "bus.res.query.srv.active - " << cyng::io::to_str(frame));

		auto const data = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] source
			std::uint64_t,			//	[1] sequence
			boost::uuids::uuid,		//	[2] websocket tag
			std::uint16_t,			//	[3] list number
			std::string,			//	[4] server id (key)
			std::string,			//	[5] meter
			std::string,			//	[6] device class
			std::chrono::system_clock::time_point,	//	[7] last status update
			std::uint32_t			//	[8] server type
		>(frame);

		//
		//	reformatting timestamp
		//
		auto stp = (cyng::chrono::same_day(std::get<7>(data), std::chrono::system_clock::now()))
			? cyng::ts_to_str(cyng::chrono::duration_of_day(std::get<7>(data)))
			: cyng::date_to_str(std::get<7>(data))
			;

		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("append")),
			cyng::param_factory("channel", "status.gateway.active"),
			cyng::param_factory("rec", cyng::tuple_factory(
				cyng::param_factory("nr", std::get<3>(data)),
				cyng::param_factory("srv", std::get<4>(data)),
				cyng::param_factory("meter", std::get<5>(data)),
				cyng::param_factory("class", std::get<6>(data)),
				cyng::param_factory("tp", stp),
				cyng::param_factory("type", std::get<8>(data))
			)));

		auto msg = cyng::json::to_string(tpl);
		connection_manager_.ws_msg(std::get<2>(data), msg);

	}


	void dispatcher::res_query_firmware(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "bus.res.query.firmware - " << cyng::io::to_str(frame));

		auto const data = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] source
			std::uint64_t,			//	[1] sequence
			boost::uuids::uuid,		//	[2] websocket tag
			std::uint32_t,			//	[3] list number
			std::string,			//	[4] server id (key)
			std::string,			//	[5] section
			std::string,			//	[6] version
			bool					//	[7] active
		>(frame);

		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("append")),
			cyng::param_factory("channel", "status.gateway.firmware"),
			cyng::param_factory("rec", cyng::tuple_factory(
				cyng::param_factory("nr", std::get<3>(data)),
				cyng::param_factory("srv", std::get<4>(data)),
				cyng::param_factory("section", std::get<5>(data)),
				cyng::param_factory("version", std::get<6>(data)),
				cyng::param_factory("active", std::get<7>(data))
			)));

		auto msg = cyng::json::to_string(tpl);
		connection_manager_.ws_msg(std::get<2>(data), msg);
	}

	void dispatcher::res_query_memory(cyng::context& ctx)
	{
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, "bus.res.query.memory - " << cyng::io::to_str(frame));

		auto const data = cyng::tuple_cast<
			boost::uuids::uuid,		//	[0] source
			std::uint64_t,			//	[1] sequence
			boost::uuids::uuid,		//	[2] websocket tag
			std::string,			//	[3] server id (key)
			std::uint8_t,			//	[4] mirror
			std::uint8_t			//	[5] tmp
		>(frame);

		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("append")),
			cyng::param_factory("channel", "status.gateway.memory"),
			cyng::param_factory("rec", cyng::tuple_factory(
				cyng::param_factory("srv", std::get<3>(data)),
				cyng::param_factory("mirror", std::get<4>(data)),
				cyng::param_factory("tmp", std::get<5>(data))
			)));

		auto msg = cyng::json::to_string(tpl);
		connection_manager_.ws_msg(std::get<2>(data), msg);
	}

	void dispatcher::subscribe(cyng::store::db& db)
	{
		db.get_listener("TDevice"
			, std::bind(&dispatcher::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&dispatcher::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&dispatcher::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&dispatcher::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		db.get_listener("TGateway"
			, std::bind(&dispatcher::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&dispatcher::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&dispatcher::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&dispatcher::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		db.get_listener("TLoRaDevice"
			, std::bind(&dispatcher::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&dispatcher::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&dispatcher::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&dispatcher::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		db.get_listener("TMeter"
			, std::bind(&dispatcher::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&dispatcher::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&dispatcher::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&dispatcher::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		db.get_listener("_Session"
			, std::bind(&dispatcher::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&dispatcher::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&dispatcher::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&dispatcher::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		db.get_listener("_Target"
			, std::bind(&dispatcher::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&dispatcher::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&dispatcher::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&dispatcher::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		db.get_listener("_Connection"
			, std::bind(&dispatcher::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&dispatcher::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&dispatcher::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&dispatcher::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		db.get_listener("_Cluster"
			, std::bind(&dispatcher::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&dispatcher::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&dispatcher::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&dispatcher::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		db.get_listener("_Config"
			, std::bind(&dispatcher::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&dispatcher::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&dispatcher::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&dispatcher::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		db.get_listener("_SysMsg"
			, std::bind(&dispatcher::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&dispatcher::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&dispatcher::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&dispatcher::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		db.get_listener("_CSV"
			, std::bind(&dispatcher::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&dispatcher::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&dispatcher::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&dispatcher::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

		CYNG_LOG_INFO(logger_, "db has " << db.num_all_slots() << " connected slots");

	}

	void dispatcher::sig_ins(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::table::data_type const& data
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		cyng::table::record rec(tbl->meta_ptr(), key, data, gen);

		if (boost::algorithm::equals(tbl->meta().get_name(), "TDevice"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "config.device"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("config.device", msg);

			update_channel("table.device.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "TGateway"))
		{
			//	data: {"cmd": "insert", "channel": "config.device", "rec": {"key": {"pk":"0b5c2a64-5c48-48f1-883b-e5be3a3b1e3d"}, "data": {"creationTime":"2018-02-04 15:31:34.00000000","descr":"comment #55","enabled":true,"id":"ID","msisdn":"1055","name":"device-55","pwd":"crypto","query":6,"vFirmware":"v55"}, "gen": 55}}
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "config.gateway"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("config.gateway", msg);

			update_channel("table.gateway.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "TLoRaDevice"))
		{
			//	
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "config.lora"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("config.lora", msg);

			update_channel("table.LoRa.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Session"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "status.session"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.session", msg);

			update_channel("table.session.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Target"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "status.target"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.target", msg);

			update_channel("table.target.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Connection"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "status.connection"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.connection", msg);

			update_channel("table.connection.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Cluster"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "status.cluster"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.cluster", msg);

			update_channel("table.cluster.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Config"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "config.sys"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("config.sys", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_SysMsg"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "monitor.msg"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("monitor.msg", msg);

			update_channel("table.msg.count", tbl->size());

		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_CSV"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("insert")),
				cyng::param_factory("channel", "task.csv"),
				cyng::param_factory("rec", rec.convert()));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("task.csv", msg);

			update_channel("table.csv.count", tbl->size());

		}
		else
		{
			CYNG_LOG_WARNING(logger_, "sig.ins - unknown table "
				<< tbl->meta().get_name());
		}
	}

	void dispatcher::sig_del(cyng::store::table const* tbl, cyng::table::key_type const& key, boost::uuids::uuid source)
	{
		if (boost::algorithm::equals(tbl->meta().get_name(), "TDevice"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("delete")),
				cyng::param_factory("channel", "config.device"),
				cyng::param_factory("key", key));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("config.device", msg);

			update_channel("table.device.count", tbl->size());

		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "TGateway"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("delete")),
				cyng::param_factory("channel", "config.gateway"),
				cyng::param_factory("key", key));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("config.gateway", msg);
			update_channel("table.gateway.count", tbl->size());

		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Session"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("delete")),
				cyng::param_factory("channel", "status.session"),
				cyng::param_factory("key", key));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.session", msg);

			update_channel("table.session.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Target"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("delete")),
				cyng::param_factory("channel", "status.target"),
				cyng::param_factory("key", key));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.target", msg);

			update_channel("table.target.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Connection"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("delete")),
				cyng::param_factory("channel", "status.connection"),
				cyng::param_factory("key", key));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.connection", msg);

			update_channel("table.connection.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Cluster"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("delete")),
				cyng::param_factory("channel", "status.cluster"),
				cyng::param_factory("key", key));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.cluster", msg);

			update_channel("table.cluster.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_SysMsg"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("delete")),
				cyng::param_factory("channel", "monitor.msg"),
				cyng::param_factory("key", key));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("monitor.msg", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_CSV"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("delete")),
				cyng::param_factory("channel", "task.csv"),
				cyng::param_factory("key", key));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("task.csv", msg);
		}
		else
		{
			CYNG_LOG_ERROR(logger_, "sig.del - unknown table "
				<< tbl->meta().get_name());
		}
	}

	void dispatcher::sig_clr(cyng::store::table const* tbl, boost::uuids::uuid source)
	{
		if (boost::algorithm::equals(tbl->meta().get_name(), "TDevice"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "config.device"));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("config.device", msg);

			update_channel("table.device.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "TGateway"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "config.gateway"));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("config.gateway", msg);
			update_channel("table.gateway.count", tbl->size());

		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "TMeter"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "config.meter"));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("config.meter", msg);
			update_channel("table.meter.count", tbl->size());

		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Session"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "status.session"));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.session", msg);

			update_channel("table.session.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Target"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "status.target"));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.target", msg);

			update_channel("table.target.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Connection"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "status.connection"));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.connection", msg);

			update_channel("table.connection.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Cluster"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "status.cluster"));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.cluster", msg);

			update_channel("table.cluster.count", tbl->size());
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_SysMsg"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "monitor.msg"));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("monitor.msg", msg);

		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Config"))
		{
			//	ToDo: Are there listener of table _Config?
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_CSV"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("clear")),
				cyng::param_factory("channel", "task.csv"));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("task.csv", msg);
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

		//
		//	convert attribute to parameter (as map)
		//
		auto pm = tbl->meta().to_param_map(attr);

		if (boost::algorithm::equals(tbl->meta().get_name(), "TDevice"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", "config.device"),
				cyng::param_factory("key", key),
				cyng::param_factory("value", pm));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("config.device", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "TGateway"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", "config.gateway"),
				cyng::param_factory("key", key),
				cyng::param_factory("value", pm));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("config.gateway", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Session"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", "status.session"),
				cyng::param_factory("key", key),
				cyng::param_factory("value", pm));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.session", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Target"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", "status.target"),
				cyng::param_factory("key", key),
				cyng::param_factory("value", pm));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.target", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Connection"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", "status.connection"),
				cyng::param_factory("key", key),
				cyng::param_factory("value", pm));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.connection", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Cluster"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", "status.cluster"),
				cyng::param_factory("key", key),
				cyng::param_factory("value", pm));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("status.cluster", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_Config"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", "config.system"),
				cyng::param_factory("key", key),
				cyng::param_factory("value", pm));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("config.system", msg);
		}
		else if (boost::algorithm::equals(tbl->meta().get_name(), "_CSV"))
		{
			auto tpl = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("modify")),
				cyng::param_factory("channel", "task.csv"),
				cyng::param_factory("key", key),
				cyng::param_factory("value", pm));

			auto msg = cyng::json::to_string(tpl);
			connection_manager_.push_event("task.csv", msg);
		}
		else
		{
			CYNG_LOG_WARNING(logger_, "sig.mode - unknown table "
				<< tbl->meta().get_name());

		}
	}

	void dispatcher::update_channel(std::string const& channel, std::size_t size)
	{
		auto tpl = cyng::tuple_factory(
			cyng::param_factory("cmd", std::string("update")),
			cyng::param_factory("channel", channel),
			cyng::param_factory("value", size));

		auto msg = cyng::json::to_string(tpl);
		connection_manager_.push_event(channel, msg);
	}

	void dispatcher::subscribe_channel(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		if (boost::algorithm::starts_with(channel, "config.device"))
		{
			subscribe(db, "TDevice", channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "config.gateway"))
		{
			subscribe(db, "TGateway", channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "config.lora"))
		{
			subscribe(db, "TLoRaDevice", channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "config.system"))
		{
			subscribe(db, "_Config", channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "status.session"))
		{
			subscribe(db, "_Session", channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "status.target"))
		{
			subscribe(db, "_Target", channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "status.connection"))
		{
			subscribe(db, "_Connection", channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "status.cluster"))
		{
			subscribe(db, "_Cluster", channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "table.device.count"))
		{
			subscribe_table_device_count(db, channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "table.gateway.count"))
		{
			subscribe_table_gateway_count(db, channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "table.meter.count"))
		{
			subscribe_table_meter_count(db, channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "table.session.count"))
		{
			subscribe_table_session_count(db, channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "table.target.count"))
		{
			subscribe_table_target_count(db, channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "table.connection.count"))
		{
			subscribe_table_connection_count(db, channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "table.msg.count"))
		{
			subscribe_table_msg_count(db, channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "table.LoRa.count"))
		{
			subscribe_table_LoRa_count(db, channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "monitor.msg"))
		{
			subscribe(db, "_SysMsg", channel, tag);
		}
		else if (boost::algorithm::starts_with(channel, "task.csv"))
		{
			subscribe(db, "_CSV", channel, tag);
		}
		else
		{
			CYNG_LOG_WARNING(logger_, "ws.read - unknown subscribe channel [" << channel << "]");
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
		else
		{
			CYNG_LOG_WARNING(logger_, "ws.read - unknown update channel [" << channel << "]");
		}
	}

	void dispatcher::update_sys_cpu_usage_total(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		db.access([&](cyng::store::table* tbl) {
			const auto rec = tbl->lookup(cyng::table::key_generator("cpu:load"));
			if (!rec.empty())
			{

				CYNG_LOG_WARNING(logger_, cyng::io::to_str(rec["value"]));
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

		auto msg = cyng::json::to_string(tpl);

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

		auto msg = cyng::json::to_string(tpl);

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

		auto msg = cyng::json::to_string(tpl);

		connection_manager_.ws_msg(tag, msg);
	}

	void dispatcher::subscribe(cyng::store::db& db, std::string table, std::string const& channel, boost::uuids::uuid tag)
	{
		//
		//	install channel
		//
		connection_manager_.add_channel(tag, channel);

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

				auto msg = cyng::json::to_string(tpl);
				connection_manager_.ws_msg(tag, msg);

				++idx;

				//
				//	calculate charge status in percent
				//
				const auto prev_percent = percent;
				percent = (100u * idx) / size;

				if (prev_percent != percent) {
					display_loading_level(tag, percent, channel);
					//
					//	give GUI a change to refresh
					//
					std::this_thread::sleep_for(std::chrono::milliseconds(120));
				}

				return true;	//	continue
			});
			BOOST_ASSERT(counter == 0);
			boost::ignore_unused(counter);	//	release version
			CYNG_LOG_INFO(logger_, tbl->size() << ' ' << tbl->meta().get_name() << " records sent");

			//
			//	inform client that data upload is finished
			//
			display_loading_icon(tag, false, channel);
		}, cyng::store::read_access(table));
	}

	void dispatcher::subscribe_table_device_count(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		connection_manager_.add_channel(tag, channel);
		const auto size = db.size("TDevice");
		update_channel(channel, size);
	}

	void dispatcher::subscribe_table_gateway_count(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		connection_manager_.add_channel(tag, channel);
		const auto size = db.size("TGateway");
		update_channel(channel, size);
	}

	void dispatcher::subscribe_table_meter_count(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		connection_manager_.add_channel(tag, channel);
		const auto size = db.size("TMeter");
		update_channel(channel, size);
	}

	void dispatcher::subscribe_table_session_count(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		connection_manager_.add_channel(tag, channel);
		const auto size = db.size("_Session");
		update_channel(channel, size);
	}

	void dispatcher::subscribe_table_target_count(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		connection_manager_.add_channel(tag, channel);
		const auto size = db.size("_Target");
		update_channel(channel, size);
	}

	void dispatcher::subscribe_table_connection_count(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		connection_manager_.add_channel(tag, channel);
		const auto size = db.size("_Connection");
		update_channel(channel, size);
	}

	void dispatcher::subscribe_table_msg_count(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		connection_manager_.add_channel(tag, channel);
		const auto size = db.size("_SysMsg");
		update_channel(channel, size);
	}

	void dispatcher::subscribe_table_LoRa_count(cyng::store::db& db, std::string const& channel, boost::uuids::uuid tag)
	{
		connection_manager_.add_channel(tag, channel);
		const auto size = db.size("TLoRaDevice");
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
		CYNG_LOG_TRACE(logger_, "http.move - " << cyng::io::to_str(frame));

		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] source
			std::string			//	[1] target
		>(frame);

		//	send 302 - Object moved response
		connection_manager_.http_moved(std::get<0>(tpl), std::get<1>(tpl));
	}

}
