/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "bridge.h"
#include "cache.h"
#include "storage.h"

#include <smf/sml/status.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/event.h>

#include <boost/core/ignore_unused.hpp>

namespace node
{

	bridge::bridge(cache& db, storage& store)
		: cache_(db)
		, storage_(store)
	{
		//
		//	Preload cache with configuration data.
		//	Preload must be done before we connect to the cache. Otherwise
		//	we would receive our own changes.
		//
		load_configuration();

		//
		//	register as listener
		//
		connect_to_cache();
	}

	void bridge::load_configuration()
	{
		cache_.db_.access([&](cyng::store::table* tbl) {

			storage_.loop("TCfg", [&](cyng::table::record const& rec)->bool {

				auto const name = cyng::value_cast<std::string>(rec["path"], "");

				tbl->update(cyng::table::key_generator(name)
					, cyng::table::data_generator(rec["val"])
					, 1u	//	only needed for insert operations
					, cache_.get_tag());

				return true;	//	continue
			});

			//
			//	store boot time
			//
			cache_.set_config_value(tbl, "boot.time", std::chrono::system_clock::now());

		}, cyng::store::write_access("_Cfg"));
	}

	void bridge::power_return()
	{
		auto const sw = cache_.get_status_word();

		storage_.generate_op_log(sw
			, sml::LOG_CODE_09	//	0x00100023 - power return
			, sml::OBIS_CODE_PEER_SCM	//	source is SCM
			, cyng::make_buffer({ })	//	server ID
			, ""	//	target
			, 0		//	nr
			, "");	//	description
	}

	void bridge::sig_ins(cyng::store::table const*
		, cyng::table::key_type const&
		, cyng::table::data_type const&
		, std::uint64_t
		, boost::uuids::uuid)
	{

	}

	void bridge::sig_del(cyng::store::table const*, cyng::table::key_type const&, boost::uuids::uuid)
	{

	}

	void bridge::sig_clr(cyng::store::table const*, boost::uuids::uuid)
	{

	}

	void bridge::sig_mod(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::attr_t const& attr
		, std::uint64_t gen
		, boost::uuids::uuid tag)
	{
		//
		//	Get the name of the column to be modified.
		//
		auto const name = tbl->meta().get_name(attr.first);

		if (boost::algorithm::equals(tbl->meta().get_name(), "_Cfg")) {
			
			if (boost::algorithm::equals(name, "status.word")) {

				//LOG_CODE_24 = 0x02100008,	//	Ethernet - Link an KundenSchnittstelle aktiviert
				//EVT_SOURCE_39 = 0x818100000011,	//	USERIF(Interne MUC - SW - Funktion)

				//
				//	write into "op.log"
				//
				std::uint64_t sw = cyng::value_cast(attr.second, sml::status::get_initial_value());
				storage_.generate_op_log(sw
					, 0x800008	//	evt
					, sml::OBIS_CODE_PEER_USERIF	//	source
					, cyng::make_buffer({ 0x81, 0x81, 0x00, 0x00, 0x00, 0x11 })	//	server ID
					, ""	//	target
					, 0		//	nr
					, "");	//	description
			}
			else {

				//
				//	Update "TCfg"
				//
				storage_.update("TCfg", name, attr.second);
			}
		}
	}


	void bridge::connect_to_cache()
	{
		cache_.db_.get_listener("_Cfg"
			, std::bind(&bridge::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&bridge::sig_del, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&bridge::sig_clr, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&bridge::sig_mod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
	}

}
