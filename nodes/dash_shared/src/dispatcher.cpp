/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "dispatcher.h"
#include <cyng/json.h>
#include <boost/algorithm/string.hpp>

namespace node 
{
	dispatcher::dispatcher(cyng::logging::log_ptr logger, connection_manager_interface& cm)
		: logger_(logger)
		, connection_manager_(cm)
	{}

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

}
