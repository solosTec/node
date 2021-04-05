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
#include <cyng/parse/buffer.h>
#include <cyng/parse/string.h>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

	db::db(cyng::store& cache, cyng::logger logger, boost::uuids::uuid tag)
		: cache_(cache)
		, logger_(logger)
		, cfg_(cache, tag)
		, store_map_()
		, uuid_gen_()
		, source_(1)	//	ip-t source id
		, channel_(1)	//	ip-t channel id
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

		//
		//	create auto tables
		//
		init_sys_msg();

//#ifdef _DEBUG_MAIN
		//
		//	insert test device
		//
		auto const tag_01 = cyng::to_uuid("4808b333-3545-4d9c-a57a-ed19d7c0548b");
		auto b = cache_.insert("device"
			, cyng::key_generator(tag_01)
			, cyng::data_generator("IEC", "pwd", "msisdn", "descr", "id", "vFirmware", true, std::chrono::system_clock::now())
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");

		auto const tag_02 = cyng::to_uuid("7afde964-b59c-4a04-b0a3-72dd63f4b6c0");
		b = cache_.insert("device"
			, cyng::key_generator(tag_02)
			, cyng::data_generator("wM-Bus", "pwd", "msisdn", "descr", "id", "vFirmware", true, std::chrono::system_clock::now())
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");

		b = cache_.insert("meterIEC"
			, cyng::key_generator(tag_01)
			, cyng::data_generator("192.168.0.200", static_cast<std::uint16_t>(2000u), std::chrono::seconds(840))
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");

		
		b = cache_.insert("meterwMBus"
			, cyng::key_generator(tag_02)
			, cyng::data_generator(boost::asio::ip::make_address("192.168.0.200"), static_cast<std::uint16_t>(6000u), cyng::make_aes_key<cyng::crypto::aes128_size>(cyng::hex_to_buffer("6E3272357538782F413F4428472B4B62")))
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");

		//
		// onee
		// 
		auto const tag_03 = cyng::to_uuid("be3931f9-6266-44db-b2b6-bd3b94b7a563");
		b = cache_.insert("device"
			, cyng::key_generator(tag_03)
			, cyng::data_generator("MA0000000000000000000000003496219", "secret", "3496219", "C1 House 101", "", "", true, std::chrono::system_clock::now())
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");
		b = cache_.insert("meterIEC"
			, cyng::key_generator(tag_03)
			, cyng::data_generator("10.132.28.150", static_cast<std::uint16_t>(6000u), std::chrono::seconds(840))
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");
		//	meter
		b = cache_.insert("meter"
			, cyng::key_generator(tag_03)
			, cyng::data_generator(
				"01-e61e-13090016-3c-07"	//	ident nummer (i.e. 1EMH0006441734, 01-e61e-13090016-3c-07)
				, "16000913"					//	[string] meter number (i.e. 16000913) 4 bytes 
				, "MA87687686876"	//cyng::column("code", cyng::TC_STRING),		//	[string] metering code - changed at 2019-01-31
				, "MAN"			//cyng::column("maker", cyng::TC_STRING),		//	[string] manufacturer
				, std::chrono::system_clock::now()	//cyng::column("tom", cyng::TC_TIME_POINT),	//	[ts] time of manufacture
				, "11600000"	//cyng::column("vFirmware", cyng::TC_STRING),	//	[string] firmwareversion (i.e. 11600000)
				, "parametrierversion"	//cyng::column("vParam", cyng::TC_STRING),	//	[string] parametrierversion (i.e. 16A098828.pse)
				, "06441734"	//cyng::column("factoryNr", cyng::TC_STRING),	//	[string] fabrik nummer (i.e. 06441734)
				, "NXT4-S20EW-6N00-4000-5020-E50/Q"	//cyng::column("item", cyng::TC_STRING),		//	[string] ArtikeltypBezeichnung = "NXT4-S20EW-6N00-4000-5020-E50/Q"
				, "Q3/Q1"	//cyng::column("mClass", cyng::TC_STRING),	//	[string] Metrological Class: A, B, C, Q3/Q1, ...
				, boost::uuids::nil_uuid() //cyng::column("gw", cyng::TC_UUID),			//	optional gateway pk
				, "IEC"	//cyng::column("protocol", cyng::TC_STRING)	//	[string] data protocol (IEC, M-Bus, COSEM, ...)
			)
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");


		auto const tag_04 = cyng::to_uuid("41c97d7a-6e37-4c49-99cf-3c87a2a11dbc");
		b = cache_.insert("device"
			, cyng::key_generator(tag_04)
			, cyng::data_generator("MA0000000000000000000000003496225", "secret", "3496225", "C1 House 94", "", "", true, std::chrono::system_clock::now())
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");
		b = cache_.insert("meterIEC"
			, cyng::key_generator(tag_04)
			, cyng::data_generator("10.132.28.151", static_cast<std::uint16_t>(6000u), std::chrono::seconds(840))
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");

		auto const tag_05 = cyng::to_uuid("5b436e3f-adac-41d9-84fe-57b0c6207f7d");
		b = cache_.insert("device"
			, cyng::key_generator(tag_05)
			, cyng::data_generator("MA0000000000000000000000003496230", "secret", "3496230", "C1 House 94/B9", "", "", true, std::chrono::system_clock::now())
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");
		b = cache_.insert("meterIEC"
			, cyng::key_generator(tag_05)
			, cyng::data_generator("10.132.28.152", static_cast<std::uint16_t>(6000u), std::chrono::seconds(840))
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");

		auto const tag_06 = cyng::to_uuid("405473f4-edd6-4afa-8cea-c224d5918f29");
		b = cache_.insert("device"
			, cyng::key_generator(tag_06)
			, cyng::data_generator("MA0000000000000000000000004354752", "secret", "4354752", "Maison 44", "", "", true, std::chrono::system_clock::now())
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");
		b = cache_.insert("meterwMBus"
			, cyng::key_generator(tag_06)
			, cyng::data_generator(boost::asio::ip::make_address("10.132.32.142"), static_cast<std::uint16_t>(2000u), cyng::make_aes_key<cyng::crypto::aes128_size>(cyng::hex_to_buffer("2BFFCB61D7E8DC439239555D3DFE1B1D")))
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");

		//
		//	gateway
		//
		auto const tag_07 = cyng::to_uuid("28c4b783-f35d-49f1-9027-a75dbae9f5e2");
		b = cache_.insert("device"
			, cyng::key_generator(tag_07)
			, cyng::data_generator("gateway-one", "secret", "one", "labor", "", "", true, std::chrono::system_clock::now())
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");
		b = cache_.insert("gateway"
			, cyng::key_generator(tag_07)
			, cyng::data_generator("0500153B02517E"	//	server ID
				, "EMH"	//	manufacturer
				, std::chrono::system_clock::now()	//	tom
				, "06441734"
				, cyng::mac48()	//cyng::column("ifService", cyng::TC_MAC48),		//	(5) MAC of service interface
				, cyng::mac48()	//cyng::column("ifData", cyng::TC_STRING),		//	(6) MAC of WAN interface
				, "pw"		//cyng::column("pwdDef", cyng::TC_STRING),		//	(7) Default PW
				, "root"	//cyng::column("pwdRoot", cyng::TC_STRING),		//	(8) root PW
				, "A815408943050131"	//cyng::column("mbus", cyng::TC_STRING),			//	(9) W-Mbus ID (i.e. A815408943050131)
				, "operator"	//cyng::column("userName", cyng::TC_STRING),		//	(10)
				, "operator"	//cyng::column("userPwd", cyng::TC_STRING)		//	(11)
			)
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
		BOOST_ASSERT_MSG(b, "insert failed");



//#endif 
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

	void db::init_sys_msg() {
		auto const ms = config::get_store_sys_msg();
		auto const start_key = cyng::key_generator(static_cast<std::uint64_t>(0));
		cache_.create_auto_table(ms, start_key, [this](cyng::key_t const& key) {
			BOOST_ASSERT(key.size() == 1);
			return cyng::key_generator(cyng::value_cast<std::uint64_t>(key.at(0), 0) + 1);
		});

	}

	bool db::push_sys_msg(std::string msg, cyng::severity level) {
		return cache_.insert_auto("sysMsg", cyng::data_generator(std::chrono::system_clock::now(), level, msg), cfg_.get_tag());
	}

	bool db::register_target(boost::uuids::uuid tag
		, std::string name
		, std::uint16_t paket_size
		, std::uint8_t window_size) {

		//
		//	ToDo: test for duplicate target names of same session 
		//

		return cache_.insert("target"
			, cyng::key_generator(channel_++)
			, cyng::data_generator(tag
				, tag
				, name
				, tag
				, "owner"
				, paket_size
				, window_size
				, std::chrono::system_clock::now()
				, static_cast<std::uint64_t>(0)		//	px
				, static_cast<std::uint64_t>(0))
			, 1
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
			config::get_store_connection(),
			//	temporary upload data
			config::get_store_uplink_lora(),
			config::get_store_uplink_iec(),
			config::get_store_uplink_wmbus()
		};
	}

}