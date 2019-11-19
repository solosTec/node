/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "cache.h"
#include "segw.h"
#include <smf/sml/obis_db.h>
#include <smf/ipt/scramble_key_format.h>

#include <cyng/table/meta.hpp>

#include <boost/core/ignore_unused.hpp>

namespace node
{

	cache::cache(cyng::store::db& db, boost::uuids::uuid tag)
		: db_(db)
		, tag_(tag)
	{}

	boost::uuids::uuid const cache::get_tag() const
	{
		return tag_;
	}

	cyng::table::meta_map_t cache::get_meta_map()
	{
		auto vec = create_cache_meta_data();
		cyng::table::meta_map_t mm;
		for (auto tbl : vec) {
			mm.emplace(tbl->get_name(), tbl);
		}
		return mm;
	}

	void cache::set_status_word(sml::status_bits code, bool b)
	{
		//
		//	atomic status update
		//
		db_.access([&](cyng::store::table* tbl) {

			//
			//	get current status word
			//
			auto word = get_config_value(tbl, "status.word", sml::status::get_initial_value());

			//
			//	set/remove flag
			//
			node::sml::status status(word);
			status.set_flag(code, b);

			//
			//	write back to cache
			//
			set_config_value(tbl, "status.word", word);

		}, cyng::store::write_access("_Cfg"));
	}

	std::uint64_t cache::get_status_word()
	{
		return get_cfg("status.word", sml::status::get_initial_value());
	}

	bool cache::merge_cfg(std::string name, cyng::object&& val)
	{
		bool r{ false };
		db_.access([&](cyng::store::table* tbl) {

			r = tbl->merge(cyng::table::key_generator(name)
				, cyng::table::data_generator(std::move(val))
				, 1u	//	only needed for insert operations
				, tag_);

		}, cyng::store::write_access("_Cfg"));

		return r;
	}

	void cache::read_table(std::string const& name, std::function<void(cyng::store::table const*)> f)
	{
		db_.access([f](cyng::store::table const* tbl) {
			f(tbl);
		}, cyng::store::read_access(name));
	}

	void cache::read_tables(std::string const& t1, std::string const& t2, std::function<void(cyng::store::table const*, cyng::store::table const*)> f)
	{
		db_.access([f](cyng::store::table const* tbl1, cyng::store::table const* tbl2) {
			f(tbl1, tbl2);
		}, cyng::store::read_access(t1), cyng::store::read_access(t2));
	}

	void cache::write_table(std::string const& name, std::function<void(cyng::store::table*)> f)
	{
		db_.access([f](cyng::store::table* tbl) {
			f(tbl);
		}, cyng::store::write_access(name));
	}

	void cache::loop(std::string const& name, std::function<bool(cyng::table::record const&)> f)
	{
		db_.access([f](cyng::store::table const* tbl) {
			tbl->loop([f](cyng::table::record const& rec)->bool {
				return f(rec);
			});
		}, cyng::store::read_access(name));
	}

	std::string cache::get_ipt_host(std::uint8_t idx)
	{
		return get_cfg<std::string>(build_cfg_key({ sml::OBIS_CODE_ROOT_IPT_PARAM
			, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx)
			, sml::make_obis(0x81, 0x49, 0x17, 0x07, 0x00, idx) }), "");
	}

	std::uint16_t cache::get_ipt_port_target(std::uint8_t idx)
	{
		try {
			auto const service = get_cfg<std::string>(build_cfg_key({ sml::OBIS_CODE_ROOT_IPT_PARAM
				, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx)
				, sml::make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, idx) }), "26862");
			return static_cast<std::uint16_t>(std::stoul(service));
		}
		catch (std::exception const&) {
		}
		return 26862u;
	}

	std::uint16_t cache::get_ipt_port_source(std::uint8_t idx)
	{
		return get_cfg<std::uint16_t>(build_cfg_key({ sml::OBIS_CODE_ROOT_IPT_PARAM
			, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx)
			, sml::make_obis(0x81, 0x49, 0x19, 0x07, 0x00, idx) }), 0u);
	}

	std::string cache::get_ipt_user(std::uint8_t idx)
	{
		return get_cfg<std::string>(build_cfg_key({ sml::OBIS_CODE_ROOT_IPT_PARAM
			, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx)
			, sml::make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, idx) }), "");
	}

	std::string cache::get_ipt_pwd(std::uint8_t idx)
	{
		return get_cfg<std::string>(build_cfg_key({ sml::OBIS_CODE_ROOT_IPT_PARAM
			, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx)
			, sml::make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, idx) }), "");
	}

	bool cache::is_ipt_scrambled(std::uint8_t idx)
	{
		return get_cfg(build_cfg_key({ sml::OBIS_CODE_ROOT_IPT_PARAM
			, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx) }
			, "scrambled"), false);
	}

	ipt::scramble_key cache::get_ipt_sk(std::uint8_t idx)
	{
		auto const s = get_cfg<std::string>(build_cfg_key({ sml::OBIS_CODE_ROOT_IPT_PARAM
			, sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx) }
			, "sk"), "0102030405060708090001020304050607080900010203040506070809000001");

		return ipt::from_string(s);
	}

	ipt::scramble_key cache::get_ipt_sk()
	{
		return get_ipt_sk(get_ipt_master_index() + 1);
	}

	std::chrono::minutes cache::get_ipt_tcp_wait_to_reconnect()
	{
		return std::chrono::minutes(get_cfg<std::uint8_t>(build_cfg_key({ sml::OBIS_CODE_ROOT_IPT_PARAM
			, sml::OBIS_TCP_WAIT_TO_RECONNECT }), 1u));
	}

	std::uint32_t cache::get_ipt_tcp_connect_retries()
	{
		return get_cfg<std::uint32_t>(build_cfg_key({ sml::OBIS_CODE_ROOT_IPT_PARAM
			, sml::OBIS_TCP_CONNECT_RETRIES }), 3u);
	}

	bool cache::has_ipt_ssl()
	{
		return get_cfg(build_cfg_key({ sml::OBIS_CODE_ROOT_IPT_PARAM
			, sml::OBIS_HAS_SSL_CONFIG }), false);
	}

	std::uint8_t cache::get_ipt_master_index()
	{
		return get_cfg<std::uint8_t>(build_cfg_key({ sml::OBIS_CODE_ROOT_IPT_PARAM }, "master"), 0u);
	}

	ipt::master_record cache::switch_ipt_redundancy()
	{
		//
		//	This reambles the logic from the class ipt::redundancy
		//
		auto const idx = (get_ipt_master_index() == 0)
			? 1
			: 0;

		//
		//	write back
		//
		set_cfg(build_cfg_key({ sml::OBIS_CODE_ROOT_IPT_PARAM }, "master"), idx);

		//
		//	return new master config record
		//
		return get_ipt_cfg(idx + 1);
	}

	ipt::master_record cache::get_ipt_cfg(std::uint8_t idx)
	{
		//81490D0700FF:81490D070001:814917070001|1|127.0.0.1|127.0.0.1|15
		//81490D0700FF:81490D070001:81491A070001|1|26862|26862|15
		//81490D0700FF:81490D070001:8149633C0101|1|gateway|gateway|15
		//81490D0700FF:81490D070001:8149633C0201|1|gateway|gateway|15
		//81490D0700FF:81490D070001:scrambled|1|true|true|1
		//81490D0700FF:81490D070001:sk|1|0102030405060708090001020304050607080900010203040506070809000001|0102030405060708090001020304050607080900010203040506070809000001|15
		

		return ipt::master_record(get_ipt_host(idx)
			, std::to_string(get_ipt_port_target(idx))
			, get_ipt_user(idx)
			, get_ipt_pwd(idx)
			, get_ipt_sk(idx)
			, is_ipt_scrambled(idx)
			, get_ipt_tcp_wait_to_reconnect().count());
	}

	ipt::redundancy cache::get_ipt_redundancy()
	{
		ipt::master_config_t const vec{ get_ipt_cfg(1) , get_ipt_cfg(2) };		
		return ipt::redundancy(vec, get_ipt_master_index());
	}

	ipt::master_record cache::get_ipt_master()
	{
		return get_ipt_cfg(get_ipt_master_index() + 1);
	}

	//
	//	initialize static member
	//
	cyng::table::meta_map_t const cache::mm_(cache::get_meta_map());

	cyng::table::meta_vec_t create_cache_meta_data()
	{
		//
		//	SQL table scheme
		//
		cyng::table::meta_vec_t vec
		{
			//
			//	Configuration table
			//
			cyng::table::make_meta_table<1, 1>("_Cfg",
			{ "path"	//	OBIS path, ':' separated values
			, "val"		//	value
			},
			{ cyng::TC_STRING
			, cyng::TC_STRING	//	may vary
			},
			{ 128
			, 256
			}),

			//
			//	That an entry of a mbus devices exists means 
			//	the device is visible
			//
			cyng::table::make_meta_table<1, 11>("_DeviceMBUS",
			{ "serverID"	//	server/meter ID
			, "lastSeen"	//	last seen - Letzter Datensatz: 20.06.2018 14:34:22"
			, "class"		//	device class (always "---" == 2D 2D 2D)
			, "active"
			, "descr"
			//	---
			, "status"	//	"Statusinformation: 00"
			, "mask"	//	"Bitmaske: 00 00"
			, "interval"	//	"Zeit zwischen zwei Datensätzen: 49000"
							//	--- optional data
			, "pubKey"	//	Public Key: 18 01 16 05 E6 1E 0D 02 BF 0C FA 35 7D 9E 77 03"
			, "aes"		//	AES-Key
			, "user"
			, "pwd"
			},
			{ cyng::TC_BUFFER		//	server ID
			, cyng::TC_TIME_POINT	//	last seen
			, cyng::TC_STRING		//	device class
			, cyng::TC_BOOL			//	active
			, cyng::TC_STRING		//	manufacturer/description

			, cyng::TC_BUFFER		//	status (81 00 60 05 00 00)
			, cyng::TC_BUFFER		//	bit mask (81 81 C7 86 01 FF)
			, cyng::TC_UINT32		//	interval (milliseconds)
			, cyng::TC_BUFFER		//	pubKey
			, cyng::TC_AES128		//	AES 128 (16 bytes)
			, cyng::TC_STRING		//	user
			, cyng::TC_STRING		//	pwd
			},
			{ 9		//	serverID
			, 0		//	lastSeen
			, 16	//	device class
			, 0		//	active
			, 128	//	manufacturer/description

			, 0		//	status
			, 8		//	mask
			, 0		//	interval
			, 16	//	pubKey
			, 32	//	aes
			, 32	//	user
			, 32	//	pwd
			})
		};

		return vec;
	}

	void init_cache(cyng::store::db& db)
	{
		//
		//	create all tables
		//
		for (auto const& tbl : cache::mm_) {
			db.create_table(tbl.second);
		}
	}

	cyng::object get_config_obj(cyng::store::table const* tbl, std::string name) {

		if (boost::algorithm::equals(tbl->meta().get_name(), "_Cfg")) {
			auto const rec = tbl->lookup(cyng::table::key_generator(name));
			if (!rec.empty())	return rec["val"];
		}
		return cyng::make_object();
	}

}
