/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <db.h>

#include <cyng/log/record.h>
#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/parse/string.h>

#include <iostream>

//#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace smf {

	db::db(cyng::store& cache
		, cyng::logger logger
		, boost::uuids::uuid tag)
	: cfg_(cache, tag)
		, cache_(cache)
		, logger_(logger)
		, store_map_()
		, subscriptions_()
		, uidgen_()
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

		std::reverse(key.begin(), key.end());
		std::reverse(data.begin(), data.end());

		CYNG_LOG_TRACE(logger_, "[cluster] db.res.insert: "
			<< table_name
			<< " - "
			<< data);

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
			<< attr.first
			<< " => "
			<< attr.second);

		//
		//	notify all subscriptions
		// 
		std::reverse(key.begin(), key.end());
		cache_.modify(table_name
			, key
			, std::move(attr)
			, tag);
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

	void db::convert(std::string const& table_name
		, cyng::vector_t& key
		, cyng::param_map_t& data) {

		auto const pos = store_map_.find(table_name);
		if (pos != store_map_.end()) {

			//
			//	table meta data
			//
			auto const& meta = pos->second;

			std::size_t index{ 0 };
			for (auto& k : key) {
				auto const col = meta.get_column(index);
				k = convert_to_type(col.type_, k);

			}

			for (auto& e : data) {
				auto const idx = meta.get_index_by_name(e.first);
				if (idx != std::numeric_limits<std::size_t>::max()) {

					auto const col = meta.get_column(idx);
					BOOST_ASSERT(boost::algorithm::equals(col.name_, e.first));

					e.second = convert_to_type(col.type_, e.second);
				}
				else {
					CYNG_LOG_ERROR(logger_, "[db] convert: unknown column [" << e.first << "] in table " << table_name);

				}
			}
		}
		else {
			CYNG_LOG_ERROR(logger_, "[db] convert: unknown table " << table_name);
		}
	}

	cyng::vector_t db::generate_new_key(std::string const& table_name, cyng::vector_t&& key, cyng::param_map_t const& data) {

		if (boost::algorithm::equals(table_name, "device")
			|| boost::algorithm::equals(table_name, "meter")
			|| boost::algorithm::equals(table_name, "meterIEC")
			|| boost::algorithm::equals(table_name, "meterwMBus")
			|| boost::algorithm::equals(table_name, "gateway")
			|| boost::algorithm::equals(table_name, "loRaDevice")
			|| boost::algorithm::equals(table_name, "guiUser")
			|| boost::algorithm::equals(table_name, "location")
			) {
			return cyng::key_generator(uidgen_());
		}
		CYNG_LOG_WARNING(logger_, "[db] cannot generate pk for table " << table_name);
		return key;
	}

	cyng::data_t db::complete(std::string const& table_name, cyng::param_map_t&& pm) {

		cyng::data_t data;
		auto const pos = store_map_.find(table_name);
		if (pos != store_map_.end()) {

			//
			//	table meta data
			//
			auto const& meta = pos->second;
			meta.loop([&](std::size_t, cyng::column const& col, bool pk)->void {

				if (!pk) {
					auto pos = pm.find(col.name_);
					if (pos != pm.end()) {

						//
						//	column found
						//	convert to expected data type
						//
						data.push_back(convert_to_type(col.type_, pos->second));

					}
					else {
						//
						//	column is missing
						//	generate a default value
						//
						data.push_back(generate_empty_value(col.type_));
					}
				}

				});

		}
		CYNG_LOG_ERROR(logger_, "[db] complete: unknown table " << table_name);
		return data;
	}

	cyng::object db::generate_empty_value(cyng::type_code tc) {
		switch (tc) {
		case cyng::TC_UUID:			return cyng::make_object(boost::uuids::nil_uuid());
		case cyng::TC_TIME_POINT:	return cyng::make_object(std::chrono::system_clock::now());
		case cyng::TC_IP_ADDRESS:	return cyng::make_object(boost::asio::ip::address());
		case cyng::TC_AES128:		return cyng::make_object(cyng::to_aes_key<cyng::crypto::aes128_size>("0000000000000000"));
		case cyng::TC_AES192:		return cyng::make_object(cyng::to_aes_key<cyng::crypto::aes192_size>("000000000000000000000000"));
		case cyng::TC_AES256:		return cyng::make_object(cyng::to_aes_key<cyng::crypto::aes256_size>("00000000000000000000000000000000"));

		case cyng::TC_BUFFER: return cyng::make_object(cyng::buffer_t());
		case cyng::TC_VERSION: return cyng::make_object(cyng::version());
		case cyng::TC_REVISION: return cyng::make_object(cyng::revision());
		case cyng::TC_OP: return cyng::make_object(cyng::op::NOOP);
		case cyng::TC_SEVERITY: return cyng::make_object(cyng::severity::LEVEL_INFO);
		case cyng::TC_MAC48: return cyng::make_object(cyng::mac48());
		case cyng::TC_MAC64: return cyng::make_object(cyng::mac64());
		case cyng::TC_PID: return cyng::make_object(cyng::pid());
		case cyng::TC_OBIS: return cyng::make_object(cyng::obis());
		case cyng::TC_EDIS: return cyng::make_object(cyng::edis());

		case cyng::TC_UINT8:		return cyng::make_object<std::uint8_t>(0);
		case cyng::TC_UINT16:		return cyng::make_object<std::uint16_t>(0);
		case cyng::TC_UINT32:		return cyng::make_object<std::uint32_t>(0);
		case cyng::TC_UINT64:		return cyng::make_object<std::uint64_t>(0);
		case cyng::TC_INT8:			return cyng::make_object<std::int8_t>(0);
		case cyng::TC_INT16:		return cyng::make_object<std::int16_t>(0);
		case cyng::TC_INT32:		return cyng::make_object<std::int32_t>(0);
		case cyng::TC_INT64:		return cyng::make_object<std::int64_t>(0);

		case cyng::TC_STRING:		return cyng::make_object("");
		case cyng::TC_NANO_SECOND:	return cyng::make_object(std::chrono::nanoseconds(0));
		case cyng::TC_MICRO_SECOND:	return cyng::make_object(std::chrono::microseconds(0));
		case cyng::TC_MILLI_SECOND:	return cyng::make_object(std::chrono::milliseconds(0));
		case cyng::TC_SECOND:		return cyng::make_object(std::chrono::seconds(0));
		case cyng::TC_MINUTE:		return cyng::make_object(std::chrono::minutes(0));
		case cyng::TC_HOUR:			return cyng::make_object(std::chrono::hours(0));

		default:
			break;
		}
		return cyng::make_object();
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
		db::rel{"session", "status.session", "table.session.count"},
		db::rel{"connection", "status.connection", "table.connection.count"}
		//db::rel{"sysMsg", "monitor.msg", "table.msg.count"}
		// 
		//db::rel{"TMeterAccess", "config.meterwMBus", "table.meterwMBus.count"},
		//db::rel{"TBridge", "config.bridge", "table.bridge.count"},
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
			config::get_store_session(),
			config::get_store_connection()
			//config::get_store_sys_msg()

		};
	}

	cyng::object convert_to_type(cyng::type_code tc, cyng::object& obj) {
		switch (tc) {
		case cyng::TC_UUID:			return convert_to_uuid(obj);
		case cyng::TC_TIME_POINT:	return convert_to_tp(obj);
		case cyng::TC_IP_ADDRESS:	return convert_to_ip_address(obj);
		case cyng::TC_AES128:		return convert_to_aes128(obj);
		case cyng::TC_AES192:		return convert_to_aes192(obj);
		case cyng::TC_AES256:		return convert_to_aes256(obj);

		case cyng::TC_UINT8:		return convert_to_numeric<std::uint8_t>(obj);
		case cyng::TC_UINT16:		return convert_to_numeric<std::uint16_t>(obj);
		case cyng::TC_UINT32:		return convert_to_numeric<std::uint32_t>(obj);
		case cyng::TC_UINT64:		return convert_to_numeric<std::uint64_t>(obj);
		case cyng::TC_INT8:			return convert_to_numeric<std::int8_t>(obj);
		case cyng::TC_INT16:		return convert_to_numeric<std::int16_t>(obj);
		case cyng::TC_INT32:		return convert_to_numeric<std::int32_t>(obj);
		case cyng::TC_INT64:		return convert_to_numeric<std::int64_t>(obj);

		default:
			break;
		}
		return obj;
	}

	cyng::object convert_to_uuid(cyng::object& obj) {
		//BOOST_ASSERT(obj.rtti().tag() == cyng::TC_STRING);
		if (obj.rtti().tag() == cyng::TC_STRING) {
			auto const str = cyng::io::to_plain(obj);
			return cyng::make_object(cyng::to_uuid(str));
		}
		else if (obj.rtti().tag() == cyng::TC_UUID) {
			return obj;	//	nothing to do
		}
		return  cyng::make_object(boost::uuids::nil_uuid());
	}

	cyng::object convert_to_tp(cyng::object& obj) {
		BOOST_ASSERT(obj.rtti().tag() == cyng::TC_STRING);
		auto const str = cyng::io::to_plain(obj);

		return cyng::make_object(cyng::to_tp_iso8601(str));

	}

	cyng::object convert_to_ip_address(cyng::object& obj) {
		BOOST_ASSERT(obj.rtti().tag() == cyng::TC_STRING);
		auto const str = cyng::io::to_plain(obj);

		return cyng::make_object(cyng::to_ip_address(str));
	}

	cyng::object convert_to_aes128(cyng::object& obj) {
		BOOST_ASSERT(obj.rtti().tag() == cyng::TC_STRING);
		auto const str = cyng::io::to_plain(obj);
		return cyng::make_object(cyng::to_aes_key<cyng::crypto::aes128_size>(str));
	}
	cyng::object convert_to_aes192(cyng::object& obj) {
		BOOST_ASSERT(obj.rtti().tag() == cyng::TC_STRING);
		auto const str = cyng::io::to_plain(obj);
		return cyng::make_object(cyng::to_aes_key<cyng::crypto::aes192_size>(str));
	}
	cyng::object convert_to_aes256(cyng::object& obj) {
		BOOST_ASSERT(obj.rtti().tag() == cyng::TC_STRING);
		auto const str = cyng::io::to_plain(obj);
		return cyng::make_object(cyng::to_aes_key<cyng::crypto::aes256_size>(str));
	}


}


