/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <db.h>
#include <smf.h>

#include <cyng/log/record.h>
#include <cyng/io/ostream.h>
#include <cyng/sys/process.h>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

	db::db(cyng::store& cache, cyng::logger logger, boost::uuids::uuid tag)
		: cache_(cache)
		, logger_(logger)
		, cfg_(cache, tag)
		, store_map_()
		, uuid_gen_()
		, source_(1)	//	ip-t source id
	{}

	void db::init(cyng::param_map_t const& session_cfg) {

		//
		//	initialize cache
		//
		auto const vec = get_store_meta_data();
		for (auto const m : vec) {
			CYNG_LOG_INFO(logger_, "init table [" << m.get_name() << "]");
			store_map_.emplace(m.get_name(), m);
			cache_.create_table(m);
		}

		BOOST_ASSERT(vec.size() == cache_.size());

		//
		//	set start values
		//
		set_start_values(session_cfg);

#ifdef _DEBUG_MAIN
		//
		//	insert test device
		//
		auto const tag_01 = uuid_gen_();
		cache_.insert("device"
			, cyng::key_generator(tag_01)
			, cyng::data_generator("name", "pwd", "msisdn", "descr", "id", "vFirmware", true, std::chrono::system_clock::now())
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());

		auto const tag_02 = uuid_gen_();
		cache_.insert("device"
			, cyng::key_generator(tag_02)
			, cyng::data_generator("wM-Bus", "pwd", "msisdn", "descr", "id", "vFirmware", true, std::chrono::system_clock::now())
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());

		cache_.insert("meterIEC"
			, cyng::key_generator(tag_01)
			, cyng::data_generator("192.168.0.200", static_cast<std::uint16_t>(2000u),std::chrono::seconds(840))
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());

		
		cache_.insert("meterwMBus"
			, cyng::key_generator(tag_02)
			, cyng::data_generator(boost::asio::ip::make_address("192.168.0.200"), static_cast<std::uint16_t>(6000u))
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
#endif 
	}

	void db::set_start_values(cyng::param_map_t const& session_cfg) {

		cfg_.set_value("startup", std::chrono::system_clock::now());

		cfg_.set_value("smf-version", SMF_VERSION_NAME);
		cfg_.set_value("boost-version", SMF_BOOST_VERSION);
		cfg_.set_value("ssl-version", SMF_OPEN_SSL_VERSION);

		//
		//	load session configuration from config file
		//
		for (auto const& param : session_cfg) {
			CYNG_LOG_TRACE(logger_, "session configuration " << param.first << ": " << param.second);
			cfg_.set_value(param.first, param.second);

		}

		//
		//	main node as cluster member
		//
		insert_cluster_member(cfg_.get_tag()
			, "main"
			, cyng::version(SMF_VERSION_MAJOR, SMF_VERSION_MINOR)
			, boost::asio::ip::tcp::endpoint()
			, cyng::sys::get_process_id());

	}

	bool db::insert_cluster_member(boost::uuids::uuid tag
		, std::string class_name
		, cyng::version v
		, boost::asio::ip::tcp::endpoint ep
		, cyng::pid pid) {

		return cache_.insert("cluster"
			, cyng::key_generator(tag)
			, cyng::data_generator(class_name
				, std::chrono::system_clock::now()
				, v
				, static_cast<std::uint64_t>(0)
				, std::chrono::microseconds(0)
				, ep
				, pid)
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
	}

	bool db::remove_cluster_member(boost::uuids::uuid tag) {
		return cache_.erase("cluster", cyng::key_generator(tag), cfg_.get_tag());
	}

	bool db::insert_pty(boost::uuids::uuid tag
		, boost::uuids::uuid peer
		, boost::uuids::uuid dev
		, std::string const& name
		, std::string const& pwd
		, boost::asio::ip::tcp::endpoint ep
		, std::string const& data_layer) {

		return cache_.insert("session"
			, cyng::key_generator(tag)
			, cyng::data_generator(peer
				, name
				, ep
				, dev	//	device
				, source_++	//	source
				, std::chrono::system_clock::now()	//	login time
				, boost::uuids::uuid()	//	rTag
				, "ipt"	//	player
				, data_layer
				, static_cast<std::uint64_t>(0)
				, static_cast<std::uint64_t>(0)
				, static_cast<std::uint64_t>(0))
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());

	}

	bool db::remove_pty(boost::uuids::uuid tag) {
		return cache_.erase("session", cyng::key_generator(tag), cfg_.get_tag());
	}

	std::size_t db::remove_pty_by_peer(boost::uuids::uuid peer) {

		cyng::key_list_t keys;

		cache_.access([&](cyng::table* tbl_pty, cyng::table* tbl_cls) {
			tbl_pty->loop([&](cyng::record&& rec, std::size_t) -> bool {

				auto const tag = rec.value("peer", peer);
				if (tag == peer)	keys.insert(rec.key());
				return true;
				});


			//
			//	remove ptys
			//
			for (auto const& key : keys) {
				tbl_pty->erase(key, cfg_.get_tag());
			}
				
			//
			//	update cluster table
			//
			tbl_cls->erase(cyng::key_generator(peer), cfg_.get_tag());

			}, cyng::access::write("session"), cyng::access::write("cluster"));

		return keys.size();
	}

	std::size_t db::update_pty_counter(boost::uuids::uuid peer) {

		std::size_t count{ 0 };
		cache_.access([&](cyng::table const* tbl_pty, cyng::table* tbl_cls) {
			tbl_pty->loop([&](cyng::record&& rec, std::size_t) -> bool {

				auto const tag = rec.value("peer", peer);
				if (tag == peer)	count++;
				return true;
				});

			//
			//	update cluster table
			//
			//tbl_cls->

			}, cyng::access::read("session"), cyng::access::write("cluster"));
		return count;
	}

	std::pair<boost::uuids::uuid, bool> 
		db::lookup_device(std::string const& name, std::string const& pwd) {

		boost::uuids::uuid tag = boost::uuids::nil_uuid();
		bool enabled = false;

		cache_.access([&](cyng::table const* tbl) {
			tbl->loop([&](cyng::record&& rec, std::size_t) -> bool {

				auto const n = rec.value("name", "");
				auto const p = rec.value("pwd", "");
				if (boost::algorithm::equals(name, n)
					&& boost::algorithm::equals(pwd, p)) {

					CYNG_LOG_INFO(logger_, "login [" << name << "] => [" << rec.value("pwd", "") << "]");

					tag = rec.value("tag", tag);
					enabled = rec.value("enabled", false);
					return false;
				}
				return true;
				});
			}, cyng::access::read("device"));

		return std::make_pair(tag, enabled);
	}

	bool db::insert_device(boost::uuids::uuid tag
		, std::string const& name
		, std::string const& pwd
		, bool enabled) {

		return cache_.insert("device"
			, cyng::key_generator(tag)
			, cyng::data_generator(name, pwd, name, "auto", "id", "1.0", enabled, std::chrono::system_clock::now())
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());

	}


	std::vector< cyng::meta_store > get_store_meta_data() {
		return {
			config::get_config(),	//	"config"
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
			config::get_store_session(),
			//	temporary upload data
			config::get_store_uplink_lora(),
			config::get_store_uplink_iec(),
			config::get_store_uplink_wmbus()
		};
	}

}