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
#include <smf/sml/srv_id_io.h>

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

	cyng::buffer_t cache::get_srv_id()
	{
		//	81 81 C7 82 04 FF
		auto mac = get_cfg(sml::OBIS_CODE_SERVER_ID.to_str(), cyng::generate_random_mac48());
		return sml::to_gateway_srv_id(mac);
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

	void cache::clear_table(std::string const& name) {
		db_.clear(name, tag_);
	}

	void cache::loop(std::string const& name, std::function<bool(cyng::table::record const&)> f)
	{
		db_.access([f](cyng::store::table const* tbl) {
			tbl->loop([f](cyng::table::record const& rec)->bool {
				return f(rec);
				});
			}, cyng::store::read_access(name));
	}


	cyng::crypto::aes_128_key cache::get_aes_key(cyng::buffer_t srv)
	{
		cyng::crypto::aes_128_key key;
		read_table("_DeviceMBUS", [&](cyng::store::table const* tbl) -> void {
			auto const obj = tbl->lookup(cyng::table::key_generator(srv), "aes");
			if (obj.is_null()) {

				auto const id = sml::from_server_id(srv);

				//
				//	preconfigured values
				//
				if (boost::algorithm::equals(id, "01-e61e-29436587-bf-03") ||
					boost::algorithm::equals(id, "01-e61e-13090016-3c-07")) {
					key.key_ = { 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8, 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8 };
				}
				else if (id == "01-a815-74314504-01-02") {
					key.key_ = { 0x23, 0xA8, 0x4B, 0x07, 0xEB, 0xCB, 0xAF, 0x94, 0x88, 0x95, 0xDF, 0x0E, 0x91, 0x33, 0x52, 0x0D };
				}
				else if (boost::algorithm::equals(id, "01-e61e-79426800-02-0e") ||
					boost::algorithm::equals(id, "01-e61e-57140621-36-03")) {
					//	6140B8C066EDDE3773EDF7F8007A45AB
					key.key_ = { 0x61, 0x40, 0xB8, 0xC0, 0x66, 0xED, 0xDE, 0x37, 0x73, 0xED, 0xF7, 0xF8, 0x00, 0x7A, 0x45, 0xAB };
				}
			}
			else {
				key = cyng::value_cast(obj, key);
			}
			});

		return key;
	}

	bool cache::update_device_table(cyng::buffer_t dev_id
		, std::string manufacturer
		, std::uint32_t status
		, std::uint8_t version
		, std::uint8_t media
		, cyng::crypto::aes_128_key aes_key
		, boost::uuids::uuid tag)
	{
		//
		//	true if device was inserted (new device)
		//
		bool r = false;
		write_table("_DeviceMBUS", [&](cyng::store::table* tbl) {

			auto const key = cyng::table::key_generator(dev_id);
			auto const now = std::chrono::system_clock::now();

#ifdef _DEBUG
			//
			//	start with some known AES keys
			//
			auto const id = sml::from_server_id(dev_id);
			if (boost::algorithm::equals(id, "01-e61e-29436587-bf-03") ||
				boost::algorithm::equals(id, "01-e61e-13090016-3c-07")) {
				aes_key.key_ = { 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8, 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8 };
			}
			else if (id == "01-a815-74314504-01-02") {
				aes_key.key_ = { 0x23, 0xA8, 0x4B, 0x07, 0xEB, 0xCB, 0xAF, 0x94, 0x88, 0x95, 0xDF, 0x0E, 0x91, 0x33, 0x52, 0x0D };
			}
			else if (boost::algorithm::equals(id, "01-e61e-79426800-02-0e") ||
				boost::algorithm::equals(id, "01-e61e-57140621-36-03")) {
				//	6140B8C066EDDE3773EDF7F8007A45AB
				aes_key.key_ = { 0x61, 0x40, 0xB8, 0xC0, 0x66, 0xED, 0xDE, 0x37, 0x73, 0xED, 0xF7, 0xF8, 0x00, 0x7A, 0x45, 0xAB };
			}
#endif

			r = tbl->insert(key
				, cyng::table::data_generator(now
					, "+++"	//	class
					, false	//	active
					, manufacturer	//	description
					, status	//	status
					, cyng::buffer_t{ 0, 0 }	//	mask
					, static_cast<std::uint32_t>(26000u)	//	interval
					, cyng::make_buffer({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 })	//	pubKey
					, aes_key	//	 AES key 
					, ""	//	user
					, "")	//	password
				, 1	//	generation
				, tag);

			//
			//	update lastSeen column
			//
			if (!r) {
				tbl->modify(key, cyng::param_factory("lastSeen", now), tag);
			}

		});


		return r;
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

			, cyng::TC_UINT32		//	status (81 00 60 05 00 00)
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
			}),

			cyng::table::make_meta_table<1, 3>("_Readout",
			{ "pk"			//	UUID
			, "serverID"	//	server/meter ID
			, "ts"			//	timestamp
			, "status"		//	M-Bus status
			},
			{ cyng::TC_UUID
			, cyng::TC_BUFFER
			, cyng::TC_TIME_POINT
			, cyng::TC_UINT8
			},
			{ 0
			, 9
			, 0
			, 0
			}),

			cyng::table::make_meta_table<2, 4>("_ReadoutData",
			{ "pk"			//	UUID => "_Readout"
			, "OBIS"		//	server/meter ID
			, "val"			//	readout value
			, "type"		//	cyng data type
			, "scaler"		//	decimal place
			, "unit"		//	physical unit
			//	future options
			//, "status"
			//, "ts"		//	timepoint
			//, "signature"
			},
			{ cyng::TC_UUID		//	pk
			, cyng::TC_BUFFER	//	OBIS
			, cyng::TC_STRING	//	val
			, cyng::TC_INT32	//	type code
			, cyng::TC_INT8		//	scaler
			, cyng::TC_UINT8	//	unit
			},
			{ 36	//	pk
			, 6		//	OBIS
			, 128	//	val
			, 0		//	type
			, 0		//	scaler
			, 0		//	unit
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
