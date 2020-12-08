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
#include <cyng/value_cast.hpp>
#include <cyng/buffer_cast.h>

#include <boost/core/ignore_unused.hpp>

namespace node
{
	cache::cache(cyng::store::db& db, boost::uuids::uuid tag)
		: db_(db)
		, tag_(tag)
		, server_id_()
		, status_word_(sml::status::get_initial_value())
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
			//	set/remove flag
			//
			sml::status status(status_word_);
			status.set_flag(code, b);

			//
			//	write back to cache
			//
			set_config_value(tbl, "status.word", status_word_);

		}, cyng::store::write_access("_Cfg"));
	}

	std::uint64_t cache::get_status_word() const
	{
		return status_word_;
	}

	bool cache::is_authorized() const
	{	
		auto copy = status_word_;
		sml::status status(copy);
		return status.is_authorized();
	}

	cyng::buffer_t cache::get_srv_id()
	{
		//	OBIS_SERVER_ID (81 81 C7 82 04 FF)
		//	this is a cached value
		return server_id_;
	}

	cyng::object cache::get_obj(std::string name) {
		return db_.get_value("_Cfg", std::string("val"), name);
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

	void cache::read_table(std::string const& name, std::function<void(cyng::store::table const*)> f) const
	{
		db_.access([f](cyng::store::table const* tbl) {
			f(tbl);
			}, cyng::store::read_access(name));
	}

	void cache::read_tables(std::string const& t1, std::string const& t2, std::function<void(cyng::store::table const*, cyng::store::table const*)> f) const
	{
		db_.access([f](cyng::store::table const* tbl1, cyng::store::table const* tbl2) {
			f(tbl1, tbl2);
			}, cyng::store::read_access(t1), cyng::store::read_access(t2));
	}

	void cache::write_table(std::string const& name, std::function<void(cyng::store::table*)> f)
	{
		db_.access([f, this](cyng::store::table* tbl) {
			db_.set_trx_state(cyng::store::trx_type::START);
			f(tbl);
			db_.set_trx_state(cyng::store::trx_type::COMMIT);
			}, cyng::store::write_access(name));
	}

	void cache::write_tables(std::string const& t1, std::string const& t2, std::function<void(cyng::store::table*, cyng::store::table*)> f)
	{
		db_.access([f, this](cyng::store::table* tbl1, cyng::store::table* tbl2) {
			db_.set_trx_state(cyng::store::trx_type::START);
			f(tbl1, tbl2);
			db_.set_trx_state(cyng::store::trx_type::COMMIT);
			}, cyng::store::write_access(t1), cyng::store::write_access(t2));
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

#if defined(NODE_CROSS_COMPILE) || defined(_DEBUG)
			//
			//	start with some known AES keys
			//
			auto const id = sml::from_server_id(dev_id);
			if (boost::algorithm::equals(id, "01-e61e-29436587-bf-03") ||
				boost::algorithm::equals(id, "01-e61e-13090016-3c-07")) {
				aes_key.key_ = { 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8, 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8 };
			}
			else if (id == "01-a815-74314504-01-02") {
				//	23A84B07EBCBAF948895DF0E9133520D
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

	cyng::store::db& cache::get_db()
	{
		return this->db_;
	}

	std::string cache::get_target(cyng::buffer_t const& srv_id, std::uint8_t nr)
	{
		return cyng::value_cast(db_.get_value("_PushOps", "target", srv_id, nr), "");
	}

	bool cache::update_ts_index(cyng::buffer_t const& srv_id, std::uint8_t nr, std::uint64_t tsidx)
	{
		return db_.modify("_PushOps"
			, cyng::table::key_generator(srv_id, nr)
			, cyng::param_factory("lowerBound", tsidx)
			, tag_);
	}

	cyng::buffer_t cache::get_meter_by_id(std::uint8_t idx) const
	{
		BOOST_ASSERT_MSG(idx != 0, "meter index starts with one");
		if (idx == 1) {
			return server_id_;
		}

		std::set<cyng::buffer_t> s;

		read_table("_DeviceMBUS", [&](cyng::store::table const* tbl) {

			tbl->loop([&](cyng::table::record const& rec) {
				s.insert(cyng::to_buffer(rec["serverID"]));
				return true;
				});

			});

		return (idx < s.size())
			? reshuffle(std::vector<cyng::buffer_t>(s.begin(), s.end())).at(idx - 1)
			: cyng::make_buffer({})
			;
	}

	std::vector<cyng::buffer_t> cache::reshuffle(std::vector<cyng::buffer_t>&& vec) const
	{
		//
		//	move server id to last position
		//
		auto const pos = std::find(std::begin(vec), std::end(vec), server_id_);
		if (pos != vec.end()) {
			//	server at first position in vector
			std::iter_swap(pos, vec.begin());
		}

		//
		// reverse ordering
		//
		std::reverse(std::begin(vec), std::end(vec));

		return vec;
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
			, "status"		//	"Statusinformation: 00"
			, "mask"		//	OBIS_ROOT_SENSOR_BITMASK - "Bitmaske: 00 00"
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

			cyng::table::make_meta_table<1, 8>("_Readout",
			{ "pk"			//	UUID
			, "serverID"	//	server/meter ID
			, "manufacturer"
			, "version"
			, "medium"
			, "dev_id"
			, "frame_type"
			, "payload"
			, "ts"			//	timestamp
			},
			{ cyng::TC_UUID			//	pk
			, cyng::TC_BUFFER		//	serverID
			, cyng::TC_STRING		//	manufacturer
			, cyng::TC_UINT8		//	version
			, cyng::TC_UINT8		//	medium
			, cyng::TC_UINT32		//	dev id
			, cyng::TC_UINT8		//	frame_type
			, cyng::TC_BUFFER		//	payload
			, cyng::TC_TIME_POINT	//	ts
			},
			{ 0
			, 9
			, 3
			, 0
			, 0
			, 0
			, 0
			, 512
			, 0
			}),

			cyng::table::make_meta_table<2, 4>("_ReadoutData",
			{ "pk"			//	UUID => "_Readout"
			, "OBIS"		//	register
			, "val"			//	readout value (as string!)
			, "type"		//	cyng data type
			, "scaler"		//	decimal place
			, "unit"		//	physical unit
			//	future options
			//, "status"		//	M-Bus status
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
			}),

			//
			//	Transaction ID
			//	example: "19041816034914837-2"
			//	Server ID
			//	example:  01 EC 4D 01 00 00 10 3C 02
			//
			cyng::table::make_meta_table<1, 3>("_trx",
			{ "trx"		//	transaction ID
			//	-- body
			, "code"		//	OBIS code
			, "serverID"	//	server ID
			, "msg"			//	optional message
			},
			{ cyng::TC_STRING		//	trx
									//	-- body
			, cyng::TC_BUFFER		//	code
			, cyng::TC_BUFFER		//	serverID
			, cyng::TC_STRING		//	msg
			},
			{ 24	//	trx
					//	-- body
			, 24	//	OBIS
			, 9		//	serverID
			, 32	//	msg
			}),

			//
			//	Controls which data are stored.
			//	81 81 C7 86 20 FF - OBIS_ROOT_DATA_COLLECTOR
			//
			cyng::table::make_meta_table<2, 4>("_DataCollector",
			{ "serverID"	//	server/meter/sensor ID
			, "nr"			//	position/number - starts with 1
							//	-- body
			, "profile"		//	[OBIS] type 1min, 15min, 1h, ... (OBIS_PROFILE)
			, "active"		//	[bool] turned on/off (OBIS_DATA_COLLECTOR_ACTIVE)
			, "maxSize"		//	[u16] max entry count (OBIS_DATA_COLLECTOR_SIZE)
			, "regPeriod"	//	[seconds] register period - if 0, recording is event-driven (OBIS_DATA_REGISTER_PERIOD)
			},
			{ cyng::TC_BUFFER		//	serverID
			, cyng::TC_UINT8		//	nr
									//	-- body
			, cyng::TC_BUFFER		//	profile
			, cyng::TC_BOOL			//	active
			, cyng::TC_UINT16		//	maxSize
			, cyng::TC_SECOND		//	regPeriod
			},
			{ 9		//	serverID
			, 0		//	nr
					//	-- body
			, 6		//	profile
			, 0		//	active
			, 0		//	maxSize
			, 0		//	regPeriod
			}),

			//
			//	Push operations
			//	81 81 C7 8A 01 FF - OBIS_PUSH_OPERATIONS
			//
			cyng::table::make_meta_table<2, 7>("_PushOps",
			{ "serverID"	//	server/meter/sensor ID
			, "nr"			//	position/number - starts with 1
							//	-- body
			, "interval"	//	[u32] (81 81 C7 8A 02 FF - PUSH_INTERVAL) push interval in seconds
			, "delay"		//	[u32] (81 81 C7 8A 03 FF - PUSH_DELAY) push delay in seconds 
			, "source"		//	[OBIS] (81 81 C7 8A 04 FF - PUSH_SOURCE) push source
			, "target"		//	[string] (81 47 17 07 00 FF - PUSH_TARGET) target name
			, "service"		//	[OBIS] (81 49 00 00 10 FF - PUSH_SERVICE) push service
			, "lowerBound"	//	[u64] last time index with successfull push
			, "tsk"			//	[u64] task ID
			},
			{ cyng::TC_BUFFER		//	serverID
			, cyng::TC_UINT8		//	nr
									//	-- body
			, cyng::TC_UINT32		//	interval
			, cyng::TC_UINT32		//	delay
			, cyng::TC_BUFFER		//	source
			, cyng::TC_STRING		//	target
			, cyng::TC_BUFFER		//	service
			, cyng::TC_UINT64		//	lowerBound
			, cyng::TC_UINT64		//	tsk
			},
			{ 9		//	serverID
			, 0		//	nr
					//	-- body
			, 0		//	interval
			, 0		//	delay
			, 6		//	source
			, 32	//	target
			, 6		//	service
			, 0		//	lowerBound
			, 0		//	tsk
			}),

			//
			//	data mirror / register - list of OBIS codes
			//	81 81 C7 8A 23 FF - DATA_COLLECTOR_REGISTER
			//
			cyng::table::make_meta_table<3, 2>("_DataMirror",
			{ "serverID"	//	server/meter/sensor ID
			, "nr"			//	reference to _DataCollector.nr
			, "reg"			//	index
							//	-- body
			, "code"		//	OBIS code
			, "active"		//	[bool] turned on/off
			},
			{ cyng::TC_BUFFER		//	serverID
			, cyng::TC_UINT8		//	nr
			, cyng::TC_UINT8		//	reg
									//	-- body
			, cyng::TC_BUFFER		//	code
			, cyng::TC_BOOL			//	active
			},
			{ 9		//	serverID
			, 0		//	nr
			, 0		//	reg
					//	-- body
			, 6		//	code
			, 0		//	active
			}),

			//
			//	ToDo: use an u8 data type as index and maintain the index to avoid
			//	gaps between the indexes and to start 0.
			//
			cyng::table::make_meta_table<1, 6, 1>("_IECDevs",
			{ "nr"			//	[u8]
			, "meterID"		//	max. 32 bytes (8181C7930AFF)
			, "address"		//	mostly the same as meterID (8181C7930CFF)
			, "descr"
			, "baudrate"	//	9600, ... (in opening sequence) (8181C7930BFF)
			, "p1"			//	login password (8181C7930DFF)
			//, "p2"		//	login password
			, "w5"			//	W5 password (reserved for national applications) (8181C7930EFF)
			},
			{ cyng::TC_UINT8		//	nr
			, cyng::TC_BUFFER		//	meterID (max. 32)
			, cyng::TC_BUFFER		//	address
			, cyng::TC_STRING		//	description
			, cyng::TC_UINT32		//	speed
			, cyng::TC_STRING		//	pwd
			, cyng::TC_STRING		//	w5
			},
			{ 0		//	nr
			, 32	//	meterID
			, 32	//	address
			, 128	//	description
			, 0		//	speed
			, 32	//	pwd
			, 32	//	w5
			}),

			//	User
			//	"nr" is index
			//
			cyng::table::make_meta_table<1, 3, 3>("_User",
				{ "user"	//	max 255
				, "role"	//	role bitmask
				, "pwd"		//	SHA256
				, "nr"
				},
				{ cyng::TC_STRING	//	user - max 255 
				, cyng::TC_UINT8	//	role
				, cyng::TC_DIGEST_SHA256	//	SHA 256 (32 bytes)
				, cyng::TC_UINT8
				},
				{ 64	//	user - max 255 
				, 0		//	role
				, 32	//	pwd
				, 0		//	nr
				}),

			//
			//	Access Rights (User)
			//
			cyng::table::make_meta_table<3, 1>("_Privileges",
				{ "user"	//	max 255
				, "meter"	//	meter/sensor/device/gateway
				, "reg"		//	register
				, "priv"	//	access right
				},
				{ cyng::TC_UINT8	//	user - max 255 
				, cyng::TC_BUFFER	//	meter/sensor
				, cyng::TC_BUFFER	//	register
				, cyng::TC_UINT8	//	priv
				},
				{ 0		//	user - max 255 
				, 9		//	meter
				, 6		//	reg
				, 0		//	priv
				})


		};

		return vec;
	}

	void init_cache(cyng::store::db& db)
	{
		//
		//	create all cache(d) tables
		//
		for (auto const& tbl : cache::mm_) {
			db.create_table(tbl.second, false);
		}
	}

	cyng::object get_config_obj(cyng::store::table const* tbl, std::string name) {

		if (boost::algorithm::equals(tbl->meta().get_name(), "_Cfg")) {
			auto const rec = tbl->lookup(cyng::table::key_generator(name));
			if (!rec.empty())	return rec["val"];
		}
		return cyng::make_object();
	}

	cyng::object get_obj(cyng::store::table const* tbl, cyng::table::key_type&& key)
	{
		BOOST_ASSERT(tbl != nullptr);
		if (tbl && boost::algorithm::equals(tbl->meta().get_name(), "_Cfg")) {
			return tbl->lookup(key, std::string("val"));
		}
		return cyng::make_object();
	}

	std::vector<cyng::buffer_t> collect_meters_of_user(cyng::store::table const* tbl, std::uint8_t nr)
	{
		//
		//	a set gives the guarantie that all entries are unique
		//
		std::set<cyng::buffer_t> s;
		if (tbl && boost::algorithm::equals(tbl->meta().get_name(), "_Privileges")) {

			tbl->loop([&](cyng::table::record const& rec) {

				auto const user = cyng::value_cast<std::uint8_t>(rec["user"], 1u);
				if (user == nr) {

					auto const id = cyng::to_buffer(rec["meter"]);
					s.emplace(id);
				}
				return true;
			});
		}

		//
		//	convert to vector
		//
		return std::vector<cyng::buffer_t>(s.begin(), s.end());
	}

}
