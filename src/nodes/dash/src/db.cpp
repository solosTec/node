/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <db.h>

#include <cyng/log/record.h>
#include <cyng/io/ostream.h>

#include <iostream>

namespace smf {

	db::db(cyng::store& cache
		, cyng::logger logger
		, boost::uuids::uuid tag)
	: cfg_(cache, tag)
		, cache_(cache)
		, logger_(logger)
		, store_map_()
		, subscriptions_()
	{}

	void db::init(std::uint64_t max_upload_size
		, std::string const& nickname
		, std::chrono::seconds timeout
		, cyng::slot_ptr slot)
	{
		//
		//	initialize cache
		//
		auto const vec = get_store_meta_data();
		for (auto const m : vec) {
			CYNG_LOG_INFO(logger_, "init table [" << m.get_name() << "]");
			store_map_.emplace(m.get_name(), m);
			cache_.create_table(m);
			cache_.connect_only(m.get_name(), slot);
		}
		BOOST_ASSERT(vec.size() == cache_.size());

		//
		//	set start values
		//
		set_start_values(max_upload_size, nickname, timeout);

	}

	void db::set_start_values(std::uint64_t max_upload_size
		, std::string const& nickname
		, std::chrono::seconds timeout) {

		cfg_.set_value("http-session-timeout", timeout);
		cfg_.set_value("http-max-upload-size", max_upload_size);	//	10.485.760 bytes
		cfg_.set_value("http-server-nickname", nickname);	//	X-ServerNickName
		cfg_.set_value("https-redirect", false);	//	redirect HTTP traffic to HTTPS port
	}

	db::rel db::by_table(std::string const& name) const {

		auto const pos = std::find_if(rel_.begin(), rel_.end(), [name](rel const& rel) {
			return boost::algorithm::equals(name, rel.table_);
			});

		return (pos == rel_.end())
			? rel()
			: *pos
			;

	}

	/**
	 * search a relation record by channel name
	 */

	db::rel db::by_channel(std::string const& name) const {
		auto pos = std::find_if(rel_.begin(), rel_.end(), [name](rel const& rel) {
			return boost::algorithm::equals(name, rel.channel_);
			});

		return (pos == rel_.end())
			? rel()
			: *pos
			;
	}

	db::rel db::by_counter(std::string const& name) const {

		auto pos = std::find_if(rel_.begin(), rel_.end(), [name](rel const& rel) {
			return boost::algorithm::equals(name, rel.counter_);
			});

		return (pos == rel_.end())
			? rel()
			: *pos
			;
	}

	void db::loop_rel(std::function<void(rel const&)> cb) const {
		for (auto const& r : rel_) {
			cb(r);
		}
	}

	void db::res_insert(std::string table_name
		, cyng::key_t  key
		, cyng::data_t  data
		, std::uint64_t gen
		, boost::uuids::uuid tag) {

		CYNG_LOG_TRACE(logger_, "[cluster] db.res.insert: "
			<< table_name
			<< " - "
			<< data);

		//std::reverse(key.begin(), key.end());
		std::reverse(data.begin(), data.end());
		cache_.insert(table_name, key, data, gen, tag);
	}

	void db::res_update(std::string table_name
		, cyng::key_t key
		, cyng::attr_t attr
		, std::uint64_t gen
		, boost::uuids::uuid tag) {

		CYNG_LOG_TRACE(logger_, "[cluster] db.res.update: "
			<< table_name
			<< " - "
			<< key);

		//
		//	ToDo: notify all subscriptions
		// 

		//
		// ToDo: implement in cyng::db
		// 
		//cache_.update(table_name
		//	, key
		//	, attr
		//	, gen
		//	, tag );

	}

	void db::res_remove(std::string table_name
		, cyng::key_t  key
		, boost::uuids::uuid tag) {

		CYNG_LOG_TRACE(logger_, "[cluster] db.res.remove: "
			<< table_name
			<< " - "
			<< key);

		//
		//	remove from cache
		//
		cache_.erase(table_name, key, tag);
	}

	void db::res_clear(std::string table_name
		, boost::uuids::uuid tag) {

		//
		//	clear table
		//
		cache_.clear(table_name, tag);

	}

	void db::add_to_subscriptions(std::string const& channel, boost::uuids::uuid tag) {
		subscriptions_.emplace(channel, tag);
	}

	std::size_t db::remove_from_subscription(boost::uuids::uuid tag) {

		std::size_t counter{ 0 };
		for (auto pos = subscriptions_.begin(); pos != subscriptions_.end(); )
		{
			if (pos->second == tag) {
				pos = subscriptions_.erase(pos);
				++counter;
			}
			else {
				++pos;
			}
		}
		return counter;
	}

	db::rel::rel(std::string table, std::string channel, std::string counter)
		: table_(table)
		, channel_(channel)
		, counter_(counter)
	{}

	bool db::rel::empty() const {
		return table_.empty();
	}

	db::array_t const db::rel_ = {
		db::rel{"device", "config.device", "table.device.count"},
		db::rel{"loRaDevice", "config.lora", "table.LoRa.count"},
		db::rel{"guiUser", "config.user", "table.user.count"},
		db::rel{"target", "status.target", "table.target.count"},
		db::rel{"cluster", "status.cluster", "table.cluster.count"},
		db::rel{"config", "config.system", ""},
		db::rel{"gateway", "config.gateway", "table.gateway.count"},
		db::rel{"meter", "config.meter", "table.meter.count"},
		db::rel{"meterIEC", "config.iec", "table.iec.count"},
		db::rel{"meterwMBus", "config.wmbus", "table.wmbus.count"},
		db::rel{"location", "config.location", "table.location.count"},
		db::rel{"session", "status.session", "table.session.count"}
		// 
		//db::rel{"TMeterAccess", "config.meterwMBus", "table.meterwMBus.count"},
		//db::rel{"TBridge", "config.bridge", "table.bridge.count"},
		//db::rel{"_Connection", "status.connection", "table.connection.count"},
		//db::rel{"_SysMsg", "monitor.msg", "table.msg.count"},
		//db::rel{"---", "config.web", ""},
		//db::rel{"_HTTPSession", "web.sessions", "table.web.count"},
		//db::rel{"_TimeSeries", "monitor.tsdb", ""},
		//db::rel{"_LoRaUplink", "monitor.lora", "table.uplink.count"},
		//db::rel{"_wMBusUplink", "monitor.wMBus", "table.wMBus.count"},
		//db::rel{"_IECUplink", "monitor.IEC", "table.IEC.count"},
		//db::rel{"_CSV", "task.csv", ""},
		//db::rel{"TGWSnapshot", "monitor.snapshot", "table.snapshot.count"},
		//db::rel{"_EventQueue", "event.queue", "table.event.queue"}

	};

	std::vector< cyng::meta_store > get_store_meta_data() {
		return {
			config::get_store_device(),
			config::get_store_meter(),
			config::get_store_meterIEC(),
			config::get_store_meterwMBus(),
			config::get_store_gateway(),
			config::get_store_lora(),
			config::get_store_gui_user(),
			config::get_store_target(),
			config::get_store_cluster(),
			config::get_store_location(),
			config::get_config(),	//	"config"
			config::get_store_session()

		};
	}

}


