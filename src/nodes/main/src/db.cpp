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

#ifdef _DEBUG_MAIN
#include <boost/uuid/random_generator.hpp>
#endif

namespace smf {

	db::db(cyng::store& cache, cyng::logger logger, boost::uuids::uuid tag)
		: cache_(cache)
		, logger_(logger)
		, cfg_(cache, tag)
		, store_map_()
	{}

	db::~db()
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
		boost::uuids::random_generator_mt19937 uidgen;
		cache_.insert("device"
			, cyng::key_generator(uidgen())
			, cyng::data_generator("name", "pwd", "msisdn", "descr", "id", "vFirmware", true, std::chrono::system_clock::now(), 6)
			, 1u	//	only needed for insert operations
			, cfg_.get_tag());
#endif 
	}

	void db::set_start_values(cyng::param_map_t const& session_cfg) {

		cfg_.set_value("startup", std::chrono::system_clock::now());

		cfg_.set_value("smf-version", SMF_VERSION_NAME);
		cfg_.set_value("boost-version", SMF_BOOST_VERSION);
		cfg_.set_value("ssl-version", SMF_OPEN_SSL_VERSION);

		for (auto const& param : session_cfg) {
			CYNG_LOG_TRACE(logger_, "session configuration " << param.first << ": " << param.second);
			cfg_.set_value(param.first, param.second);

		}
	}

	std::vector< cyng::meta_store > get_store_meta_data() {
		return {
			config::get_store_device(),
			config::get_store_lora(),
			config::get_store_gui_user(),
			config::get_store_target(),
			config::get_store_cluster(),
			config::get_config()	//	"config"
		};
	}

}