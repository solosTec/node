/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "storage_global.h"
#include "storage.h"
#include "segw.h"
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/ipt/config.h>
#include <smf/ipt/scramble_key_format.h>
#include <smf/sml/parser/obis_parser.h>
#include <smf/sml/srv_id_io.h>
#include <smf/mbus/units.h>
#include <smf/sml/protocol/reader.h>

#include <cyng/table/meta.hpp>
#include <cyng/db/interface_statement.h>
#include <cyng/table/restore.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/vector_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/buffer_cast.h>
#include <cyng/parser/mac_parser.h>
#include <cyng/io/serializer.h>
#include <cyng/util/split.h>

#include <boost/core/ignore_unused.hpp>

namespace node
{

	cyng::table::meta_vec_t create_storage_meta_data()
	{
		//
		//	SQL table scheme
		//
		return 
		{
			//
			//	Configuration table
			//
			cyng::table::make_meta_table_gen<1, 3>("TCfg",
			{ "path"	//	OBIS path, ':' separated values
			, "val"		//	value
			, "def"		//	default value
			, "type"	//	data type code (default)
			},
			{ cyng::TC_STRING
			, cyng::TC_STRING
			, cyng::TC_STRING
			, cyng::TC_UINT32
			},
			{ 128
			, 256
			, 256
			, 0
			}),

			//
			//	operation log (81 81 C7 89 E1 FF)
			//
			cyng::table::make_meta_table_gen<1, 11>("TOpLog",
				{ "ROWID"		//	index - with SQLite this prevents creating a column
								//	-- body
				, "actTime"		//	actual time
				, "regPeriod"	//	register period
				, "valTime"		//	val time
				, "status"		//	status word
				, "event"		//	event code
				, "peer"		//	peer address
				, "utc"			//	UTC time
				, "serverId"	//	server ID (meter)
				, "target"		//	target name
				, "pushNr"		//	operation number
				, "details"		//	description (DATA_PUSH_DETAILS)
				},
				{ cyng::TC_UINT64		//	index 
										//	-- body
				, cyng::TC_TIME_POINT	//	actTime
				, cyng::TC_UINT32		//	regPeriod
				, cyng::TC_TIME_POINT	//	valTime
				, cyng::TC_UINT64		//	status
				, cyng::TC_UINT32		//	event
				, cyng::TC_BUFFER		//	peer_address
				, cyng::TC_TIME_POINT	//	UTC time
				, cyng::TC_BUFFER		//	serverId
				, cyng::TC_STRING		//	target
				, cyng::TC_UINT8		//	push_nr
				, cyng::TC_STRING		//	details
				},
				{ 0		//	index
						//	-- body
				, 0		//	actTime
				, 0		//	regPeriod
				, 0		//	valTime
				, 0		//	status
				, 0		//	event
				, 13	//	peer_address
				, 0		//	utc
				, 23	//	serverId
				, 64	//	target
				, 0		//	push_nr
				, 128	//	string
				}),

			//
			//	Communication Profile
			//

			//
			//	Access Rights (User)
			//

			//
			//	Profile Configurations
			//
			create_profile_meta("TProfile_8181C78610FF"),	//	PROFILE_1_MINUTE
			create_profile_meta("TProfile_8181C78611FF"),	//	PROFILE_15_MINUTE
			create_profile_meta("TProfile_8181C78612FF"),	//	PROFILE_60_MINUTE
			create_profile_meta("TProfile_8181C78613FF"),	//	PROFILE_24_HOUR
			//create_meta_load_profile("TProfile_8181C78614FF"),	//	PROFILE_LAST_2_HOURS
			//create_meta_load_profile("TProfile_8181C78615FF"),	//	PROFILE_LAST_WEEK
			//create_meta_load_profile("TProfile_8181C78616FF"),	//	PROFILE_1_MONTH
			//create_meta_load_profile("TProfile_8181C78617FF"),	//	PROFILE_1_YEAR
			//create_meta_load_profile("TProfile_8181C78618FF"),	//	PROFILE_INITIAL

			//
			//	Readout data (one for each profile)
			//
			create_profile_storage("TStorage_8181C78610FF"),	//	PROFILE_1_MINUTE
			create_profile_storage("TStorage_8181C78611FF"),	//	PROFILE_15_MINUTE
			create_profile_storage("TStorage_8181C78612FF"),	//	PROFILE_60_MINUTE
			create_profile_storage("TStorage_8181C78613FF"),	//	PROFILE_24_HOUR
			//create_meta_data_storage("TStorage_8181C78614FF"),	//	PROFILE_LAST_2_HOURS
			//create_meta_data_storage("TStorage_8181C78615FF"),	//	PROFILE_LAST_WEEK
			//create_meta_data_storage("TStorage_8181C78616FF"),	//	PROFILE_1_MONTH
			//create_meta_data_storage("TStorage_8181C78617FF"),	//	PROFILE_1_YEAR
			//create_meta_data_storage("TStorage_8181C78618FF"),	//	PROFILE_INITIAL

			//
			//	Configuration/Status
			//	visible/active Devices
			//
			//
			//	That an entry of a mbus devices exists means 
			//	the device is visible
			//
			cyng::table::make_meta_table_gen<1, 11>("TDeviceMBUS",
				{ "serverID"	//	server/meter ID
				, "lastSeen"	//	last seen - Letzter Datensatz: 20.06.2018 14:34:22"
				, "class"		//	device class (always "---" == 2D 2D 2D)
				, "active"
				, "descr"
				//	---
				, "status"	//	"Statusinformation: 00"
				, "mask"	//	"Bitmaske: 00 00"
				, "interval"	//	"Zeit zwischen zwei Datens�tzen: 49000"
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
				, cyng::TC_UINT16		//	bit mask (81 81 C7 86 01 FF)
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
				, 0		//	mask
				, 0		//	interval
				, 16	//	pubKey
				, 32	//	aes
				, 32	//	user
				, 32	//	pwd
				}),

			//
			//	select hex(serverID), nr, gen, hex(profile), active, maxSize, regPeriod from TDataCollector;
			//
			cyng::table::make_meta_table_gen<2, 4>("TDataCollector",
				{ "serverID"	//	server/meter/sensor ID
				, "nr"			//	position/number - starts with 1
								//	-- body
				, "profile"		//	[OBIS] type 1min, 15min, 1h, ... (OBIS_PROFILE)
				, "active"		//	[bool] turned on/off (OBIS_DATA_COLLECTOR_ACTIVE)
				, "maxSize"		//	[u32] max entry count (OBIS_DATA_COLLECTOR_SIZE)
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
			//	select hex(serverID), nr, gen, interval, delay, hex(source), target, hex(service) from TPushOps;
			//
			cyng::table::make_meta_table_gen<2, 6>("TPushOps",
				{ "serverID"	//	server/meter/sensor ID
				, "nr"			//	position/number - starts with 1
								//	-- body
				, "interval"	//	[u32] (81 18 C7 8A 02 FF) push interval in seconds
				, "delay"		//	[u32] (81 18 C7 8A 03 FF) push delay in seconds 
				, "source"		//	[OBIS] (81 81 C7 8A 04 FF) push source
				, "target"		//	[string] (81 47 17 07 00 FF) target name
				, "service"		//	[OBIS] (81 49 00 00 10 FF) push service
				, "lowerBound"	//	[u64] last time index with successfull push
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
				}),

			//
			//	data mirror - list of OBIS codes
			//	81 81 C7 8A 23 FF - DATA_COLLECTOR_OBIS
			//	select hex(serverID), nr, reg, gen, hex(code), active from TDataMirror;
			//
			cyng::table::make_meta_table_gen<3, 2>("TDataMirror",
				{ "serverID"	//	server/meter/sensor ID
				, "nr"			//	reference to TDataCollector.nr
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
				})


		};
	}

	cyng::table::meta_table_ptr create_profile_meta(std::string name)
	{
		return cyng::table::make_meta_table_gen<2, 3>(name,
			{ "clientID"	//	server/meter/sensor ID
			, "tsidx"		//	time stamp index
							//	-- body
			, "actTime"		//
			, "valTime"		//	signed integer
			, "status"		//	status
			},
			{ cyng::TC_BUFFER		//	clientID
			, cyng::TC_UINT64		//	tsidx
									//	-- body
			, cyng::TC_TIME_POINT	//	actTime
			, cyng::TC_UINT32		//	valTime
			, cyng::TC_UINT32		//	status
			},
			{ 9		//	serverID
			, 0		//	tsidx
					//	-- body
			, 0		//	actTime
			, 0		//	valTime
			, 0		//	status
			});
	}

	cyng::table::meta_table_ptr create_profile_storage(std::string name)
	{
		return cyng::table::make_meta_table_gen<3, 4>(name,
			{ "clientID"	//	server/meter/sensor ID
			, "tsidx"		//	time stamp index
			, "OBIS"		//	server/meter ID
			, "val"			//	readout value
			, "type"		//	cyng data type
			, "scaler"		//	decimal place
			, "unit"		//	physical unit
			},
			{ cyng::TC_BUFFER	//	clientID
			, cyng::TC_UINT64	//	tsidx
			, cyng::TC_BUFFER	//	OBIS
								//	-- body
			, cyng::TC_STRING	//	val
			, cyng::TC_INT32	//	type code
			, cyng::TC_INT8		//	scaler
			, cyng::TC_UINT8	//	unit
			},
			{ 9		//	serverID
			, 0		//	tsidx
			, 6		//	OBIS
					//	-- body
			, 128	//	val
			, 0		//	type
			, 0		//	scaler
			, 0		//	unit
			});
	}

	bool init_storage(cyng::param_map_t&& cfg)
	{
		auto con_type = cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite"));
		cyng::db::session s(con_type);
		auto r = s.connect(cfg);
		if (r.second) {

			//
			//	connect string
			//
#ifdef _DEBUG
			std::cout
				<< "connect string: ["
				<< r.first
				<< "]"
				<< std::endl
				;
#endif

			//
			//	start transaction
			//
			s.begin();


			for (auto const& tbl : storage::mm_)
			{
#ifdef _DEBUG
				std::cout
					<< "create table  : ["
					<< tbl.first
					<< "]"
					<< std::endl
					;
#endif
				cyng::sql::command cmd(tbl.second, s.get_dialect());
				auto sql = cmd.create()();
#ifdef _DEBUG
				std::cout
					<< sql
					<< std::endl;
#endif
				s.execute(sql);
			}

			//
			//	commit
			//
			s.commit();

			return true;
		}
		else {
			std::cerr
				<< "connect ["
				<< r.first
				<< "] failed"
				<< std::endl
				;
		}
		return false;
	}

	bool transfer_config_to_storage(cyng::param_map_t&& cfg, cyng::reader<cyng::object> const& dom)
	{
		//
		//	create a database session
		//
		auto con_type = cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite"));
		cyng::db::session s(con_type);
		auto r = s.connect(cfg);
		if (r.second) {

			//
			//	start transaction
			//
			s.begin();

			//
			//	transfer IP-T configuration
			//
			{
				cyng::vector_t vec;
				vec = cyng::value_cast(dom.get("ipt"), vec);
				auto const cfg_ipt = ipt::load_cluster_cfg(vec);
				std::uint8_t idx{ 1 };
				for (auto const rec : cfg_ipt.config_) {
					//	host
					init_config_record(s, build_cfg_key({
						sml::OBIS_ROOT_IPT_PARAM,
						sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
						sml::make_obis(0x81, 0x49, 0x17, 0x07, 0x00, idx)
						}), cyng::make_object(rec.host_));

					//	target port
					init_config_record(s, build_cfg_key({
						sml::OBIS_ROOT_IPT_PARAM,
						sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
						sml::make_obis(0x81, 0x49, 0x1A, 0x07, 0x00, idx)
						}), cyng::make_object(rec.service_));

					//	source port (unused)
					init_config_record(s, build_cfg_key({
						sml::OBIS_ROOT_IPT_PARAM,
						sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
						sml::make_obis(0x81, 0x49, 0x19, 0x07, 0x00, idx)
						}), cyng::make_object(static_cast<std::uint16_t>(0u)));

					//	account
					init_config_record(s, build_cfg_key({
						sml::OBIS_ROOT_IPT_PARAM,
						sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
						sml::make_obis(0x81, 0x49, 0x63, 0x3C, 0x01, idx)
						}), cyng::make_object(rec.account_));

					//	password
					init_config_record(s, build_cfg_key({
						sml::OBIS_ROOT_IPT_PARAM,
						sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx),
						sml::make_obis(0x81, 0x49, 0x63, 0x3C, 0x02, idx)
						}), cyng::make_object(rec.pwd_));

					//	scrambled
					init_config_record(s, build_cfg_key({
						sml::OBIS_ROOT_IPT_PARAM,
						sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx)
						}, "scrambled"), cyng::make_object(rec.scrambled_));

					//	scramble key
					init_config_record(s, build_cfg_key({
						sml::OBIS_ROOT_IPT_PARAM,
						sml::make_obis(0x81, 0x49, 0x0D, 0x07, 0x00, idx)
						}, "sk"), cyng::make_object(ipt::to_string(rec.sk_)));

					// update index
					++idx;
				}

				//	ip-t reconnect time in minutes
				auto reconnect = cyng::numeric_cast<std::uint32_t>(dom["ipt-param"].get(sml::OBIS_TCP_WAIT_TO_RECONNECT.to_str()), 1u);
				init_config_record(s, build_cfg_key({
					sml::OBIS_ROOT_IPT_PARAM,
					sml::OBIS_TCP_WAIT_TO_RECONNECT
					}), cyng::make_minutes(reconnect));

				auto retries = cyng::numeric_cast<std::uint32_t>(dom["ipt-param"].get(sml::OBIS_TCP_CONNECT_RETRIES.to_str()), 3u);
				init_config_record(s, build_cfg_key({
					sml::OBIS_ROOT_IPT_PARAM,
					sml::OBIS_TCP_CONNECT_RETRIES
					}), cyng::make_object(retries));

				//	has SSL/TLS
				init_config_record(s, build_cfg_key({
					sml::OBIS_ROOT_IPT_PARAM,
					sml::OBIS_HAS_SSL_CONFIG
					}), cyng::make_object(false));

				//
				//	master index (0..1)
				//
				init_config_record(s, build_cfg_key({
					sml::OBIS_ROOT_IPT_PARAM}, "master"), cyng::make_object(static_cast<std::uint8_t>(0u)));

			}

			//
			//	transfer IEC configuration
			//
			{
				//	get a tuple/list of params
				cyng::tuple_t const tpl = cyng::to_tuple(dom.get("if-1107"));
				for (auto const& obj : tpl) {
					cyng::param_t param;
					param = cyng::value_cast(obj, param);

					auto const code = sml::to_obis(param.first);
					if (!code.is_nil()) {

						init_config_record(s, build_cfg_key({
							sml::OBIS_IF_1107,
							code
							}), param.second);
					}
					else {
						init_config_record(s, build_cfg_key({
							sml::OBIS_IF_1107
							}, param.first), param.second);
					}
				}
			}

			//
			//	transfer M-Bus configuration
			//
			{
				init_config_record(s, build_cfg_key({
					sml::OBIS_CLASS_MBUS
					}, "descr"), cyng::make_object("M-Bus Configuration"));

				//	get a tuple/list of params
				cyng::tuple_t const tpl = cyng::to_tuple(dom.get("mbus"));
				for (auto const& obj : tpl) {
					cyng::param_t param;
					param = cyng::value_cast(obj, param);

					auto const code = sml::to_obis(param.first);
					if (!code.is_nil()) {

						if (sml::OBIS_CLASS_MBUS_RO_INTERVAL == code) {
							//	[u32/seconds]
							auto const val = cyng::numeric_cast<std::uint32_t> (param.second, 3600u);
							init_config_record(s, build_cfg_key({
								sml::OBIS_CLASS_MBUS,
								code
								}), cyng::make_seconds(val));
						}
						else if (sml::OBIS_CLASS_MBUS_SEARCH_INTERVAL == code) {
							//	[u32/seconds]
							auto const val = cyng::numeric_cast<std::uint32_t> (param.second, 0u);
							init_config_record(s, build_cfg_key({
								sml::OBIS_CLASS_MBUS,
								code
								}), cyng::make_seconds(val));
						}
						else if (sml::OBIS_CLASS_MBUS_BITRATE == code) {
							//	[u32] bismask
							//	bit 1 = 150 baud
							//	bit 2 = 300 baud
							//	bit 5 = 2400 baud
							//	bit 7 = 9600 baud
							auto const val = cyng::numeric_cast<std::uint32_t> (param.second, 0u);
							init_config_record(s, build_cfg_key({
								sml::OBIS_CLASS_MBUS,
								code
								}), cyng::make_object(val));
						}
						else {
							init_config_record(s, build_cfg_key({
								sml::OBIS_CLASS_MBUS,
								code
								}), param.second);
						}
					}
					else {
						init_config_record(s, build_cfg_key({
							sml::OBIS_CLASS_MBUS
							}, param.first), param.second);
					}
				}
			}

			//
			//	transfer wireless-LMN configuration
			//	81 06 19 07 01 FF
			//
			{
				init_config_record(s, build_cfg_key({
					sml::OBIS_W_MBUS_PROTOCOL
					}, "descr"), cyng::make_object("wireless-LMN configuration (M-Bus)"));

				//	get a tuple/list of params
				cyng::tuple_t const tpl = cyng::to_tuple(dom.get("wireless-LMN"));
				for (auto const& obj : tpl) {
					cyng::param_t param;
					param = cyng::value_cast(obj, param);

					auto const code = sml::to_obis(param.first);
					if (!code.is_nil()) {

						init_config_record(s, build_cfg_key({
							sml::OBIS_W_MBUS_PROTOCOL,
							code
							}), param.second);
					}
					else if (boost::algorithm::equals(param.first, "monitor")) {
						//	int
						auto const val = cyng::numeric_cast(param.second, 30);
						init_config_record(s, build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, param.first ), cyng::make_seconds(val));
					}
					else {

						init_config_record(s, build_cfg_key({
							sml::OBIS_W_MBUS_PROTOCOL
							}, param.first), param.second);
					}
				}
			}

			//
			//	transfer wired-LMN configuration
			//	as part of IEC 62506-21 configuration
			//	81 81 C7 93 00 FF
			//
			{
				init_config_record(s, build_cfg_key({
					"rs485", "descr" 
					}), cyng::make_object("RS485 interface"));

				//	get a tuple/list of params
				cyng::tuple_t const tpl = cyng::to_tuple(dom.get("wired-LMN"));
				for (auto const& obj : tpl) {
					cyng::param_t param;
					param = cyng::value_cast(obj, param);

					if (boost::algorithm::equals(param.first, "databits")) {
						//	u8
						auto const val = cyng::numeric_cast<std::uint8_t>(param.second, 8u);
						init_config_record(s, build_cfg_key({ "rs485", param.first }), cyng::make_object(val));
					}
					else if (boost::algorithm::equals(param.first, "monitor")) {
						//	int
						auto const val = cyng::numeric_cast(param.second, 30);
						init_config_record(s, build_cfg_key({ "rs485", param.first }), cyng::make_seconds(val));
					}
					else {
						init_config_record(s, build_cfg_key({ "rs485", param.first }), param.second);
					}
				}
			}

			//
			//	transfer hardware configuration
			//
			{
				//	get a tuple/list of params
				cyng::tuple_t const tpl = cyng::to_tuple(dom.get("hardware"));
				for (auto const& obj : tpl) {
					cyng::param_t param;
					param = cyng::value_cast(obj, param);

					if (boost::algorithm::equals(param.first, "mac")) {

						//	"mac": "00:ff:90:98:57:56"
						//
						//	05 + MAC = server ID
						//
						std::string rnd_mac_str;
						{
							using cyng::io::operator<<;
							std::stringstream ss;
							ss << cyng::generate_random_mac48();
							ss >> rnd_mac_str;
						}
						auto const mac = cyng::value_cast<std::string>(param.second, rnd_mac_str);

						std::pair<cyng::mac48, bool > const r = cyng::parse_mac48(mac);
						init_config_record(s, build_cfg_key({
							sml::OBIS_SERVER_ID
						}), cyng::make_object(r.second ? r.first : cyng::generate_random_mac48()));

					}
					else if (boost::algorithm::equals(param.first, "class")) {

						//	"class": "129-129:199.130.83*255", (81 81 C7 82 53 FF)
						auto const id = cyng::value_cast<std::string>(param.second, "129-129:199.130.83*255");
						auto const r = sml::parse_obis(id);
						if (r.second) {
							init_config_record(s, build_cfg_key({
								sml::OBIS_DEVICE_CLASS
							}), cyng::make_object(r.first.to_str()));
						}
						else {
							init_config_record(s, build_cfg_key({
								sml::OBIS_DEVICE_CLASS
								}), cyng::make_object(sml::OBIS_DEV_CLASS_MUC_LAN.to_str()));
						}
					}
					else if (boost::algorithm::equals(param.first, "manufacturer")) {

						//	"manufacturer": "ACME Inc.",
						auto const id = cyng::value_cast<std::string>(param.second, "solosTec");
						init_config_record(s, build_cfg_key({
							sml::OBIS_DATA_MANUFACTURER
							}), cyng::make_object(id));

					}
					else if (boost::algorithm::equals(param.first, "serial")) {

						//	"serial": 34287691,
						std::uint32_t const serial = cyng::numeric_cast<std::uint32_t>(param.second, 10000000u);
						init_config_record(s, build_cfg_key({
							sml::OBIS_SERIAL_NR
							}), cyng::make_object(serial));

					}

				}
			}

			//
			//	SML login: accepting all/wrong server IDs
			//
			init_config_record(s, "accept-all-ids", dom.get("accept-all-ids"));

			//
			//	OBIS logging cycle in minutes
			//
			{
				auto const val = cyng::value_cast(dom.get(sml::OBIS_OBISLOG_INTERVAL.to_str()), 15);
				init_config_record(s, sml::OBIS_OBISLOG_INTERVAL.to_str(), cyng::make_minutes(val));
			}

			//
			//	shifting storage time in seconds (affects all meters)
			//
			{
				auto const val = cyng::numeric_cast<std::int32_t>(dom.get(sml::OBIS_STORAGE_TIME_SHIFT.to_str()), 0);
				init_config_record(s, sml::OBIS_STORAGE_TIME_SHIFT.to_str(), cyng::make_object(val));
			}

			{

				//
				//	map all available GPIO paths
				//
				init_config_record(s, "gpio-path", dom.get("gpio-path"));

				auto const gpio_list = cyng::vector_cast<int>(dom.get("gpio-list"), 0);
				if (gpio_list.size() != 4) {
					std::cerr 
						<< "***warning: invalid count of gpios: " 
						<< gpio_list.size()
						<< std::endl;
				}
				std::stringstream ss;
				bool initialized{ false };
				for (auto const gpio : gpio_list) {
					if (initialized) {
						ss << ' ';
					}
					else {
						initialized = true;
					}
					ss
						<< gpio
						;
				}
				init_config_record(s, "gpio-vector", cyng::make_object(ss.str()));
			}

			//
			//	transfer server configuration
			//	"server"
			//
			{
				//	get a tuple/list of params
				cyng::tuple_t const tpl = cyng::to_tuple(dom.get("server"));
				for (auto const& obj : tpl) {

					cyng::param_t param;
					param = cyng::value_cast(obj, param);

					if (boost::algorithm::equals(param.first, "address")) {
						init_config_record(s, "server:address", param.second);
					}
					else if (boost::algorithm::equals(param.first, "service")) {
						init_config_record(s, "server:service", param.second);
					}
					else if (boost::algorithm::equals(param.first, "discover")) {
						init_config_record(s, "server:discover", param.second);
					}
					else if (boost::algorithm::equals(param.first, "account")) {
						init_config_record(s, "server:account", param.second);
					}
					else if (boost::algorithm::equals(param.first, "pwd")) {
						init_config_record(s, "server:pwd", param.second);
					}
				}
			}

			//
			//	commit
			//
			s.commit();

			return true;
		}

		return false;
	}

	int clear_config_from_storage(cyng::param_map_t&& cfg, cyng::reader<cyng::object> const& dom)
	{
		//
		//	clear table TCfg
		//
		auto con_type = cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite"));
		cyng::db::session s(con_type);
		auto r = s.connect(cfg);
		if (r.second) {
			cyng::table::meta_table_ptr meta = storage::mm_.at("TCfg");
			cyng::sql::command cmd(meta, s.get_dialect());
			auto const sql = cmd.remove()();
			auto stmt = s.create_statement();
			auto const r = stmt->prepare(sql);
			if (r.second) {
				if (stmt->execute()) {
					return stmt->changes();
				}
			}
		}
		return 0;
	}

	bool dump_profile_data(cyng::param_map_t&& cfg, cyng::reader<cyng::object> const& dom, std::uint32_t profile)
	{
		//
		//	create a database session
		//
		auto con_type = cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite"));
		cyng::db::session s(con_type);
		auto r = s.connect(cfg);
		if (r.second) {

			//
			//	get SQL select
			//
			std::string const sql = get_sql_select_all(profile);
				
			std::cout
				<< "List "
				<< get_profile_name(profile)
				<< " profile ("
				<< profile
				<< ")"
				<< std::endl;

			if (!sql.empty()) {
				auto stmt = s.create_statement();
				std::pair<int, bool> r = stmt->prepare(sql);
				if (r.second) {

					//
					//	read all results
					//
					std::size_t counter{ 0 };
					while (auto res = stmt->get_result()) {

						//
						//	Convert SQL result to record
						//
						auto const size = res->column_count();
						BOOST_ASSERT(size == 9);
						auto const srv_id = cyng::to_buffer(res->get(1, cyng::TC_BUFFER, 9));
						auto const tsdix = cyng::value_cast<std::uint64_t>(res->get(2, cyng::TC_UINT64, 0), 0);
						//	3. actTime
						//	4. status
						sml::obis const code(cyng::to_buffer(res->get(5, cyng::TC_BUFFER, 6)));
						//	6. val
						auto const val = cyng::value_cast<std::string>(res->get(6, cyng::TC_STRING, 0), "");
						//	7. type
						auto const type = cyng::value_cast<std::uint32_t>(res->get(7, cyng::TC_UINT32, 0), 0);
						auto const scaler = cyng::value_cast<std::int8_t>(res->get(8, cyng::TC_INT8, 0), 0);
						auto const unit = cyng::value_cast<std::uint8_t>(res->get(9, cyng::TC_UINT8, 0), 0);

						//
						//	restore object from string and type info
						//
						auto const obj = cyng::table::restore(val, type);
						auto const read_out = sml::reader::read_value(code, scaler, unit, obj);
						
						std::cout
							<< sml::from_server_id(srv_id)
							<< ", "
							<< tsdix
							<< " - "
							<< cyng::to_str(time_index_to_time_point(profile, tsdix))
							<< ", "				
							<< cyng::io::to_str(res->get(3, cyng::TC_TIME_POINT, 0)).substr(0, 19)
							<< ", "
							<< cyng::value_cast<std::uint32_t>(res->get(4, cyng::TC_UINT32, 0), -1)
							<< ", "
							<< code.to_str()
							<< ", "
							<< val
							<< ", "
							<< cyng::io::to_str(read_out)
							<< ' '
							<< mbus::get_unit_name(unit)
							<< std::endl;

						++counter;

					}

					std::cout
						<< counter
						<< " records of "
						<< get_profile_name(profile)
						<< " profile (#"
						<< profile
						<< ") found"
						<< std::endl;

				}
			}
		}
		return r.second;
	}

	std::ostream& list_value(std::ostream& os, std::string val, std::string def)
	{
		if (boost::algorithm::equals(val, def)) {
			os << val;
		}
		else {
			os
				<< '['
				<< val
				<< '/'
				<< def
				<< ']'
				;
		}
		return os;
	}

	bool list_config_data(std::ostream& os, cyng::param_map_t&& cfg, cyng::reader<cyng::object> const& dom)
	{
		//
		//	create a database session
		//
		auto con_type = cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["type"], "SQLite"));
		cyng::db::session s(con_type);
		auto r = s.connect(cfg);
		if (r.second) {

			cyng::table::meta_table_ptr meta = storage::mm_.at("TCfg");
			cyng::sql::command cmd(meta, s.get_dialect());
			auto const sql = cmd.select().all().skip().order_by("path")();
			auto stmt = s.create_statement();
			std::pair<int, bool> r = stmt->prepare(sql);
			if (r.second) {
				std::string section{ "000000000000" };
				while (auto res = stmt->get_result()) {
					auto const path = cyng::value_cast<std::string>(res->get(1, cyng::TC_STRING, 0), "");
					auto const val = cyng::value_cast<std::string>(res->get(3, cyng::TC_STRING, 0), "");
					auto const def = cyng::value_cast<std::string>(res->get(4, cyng::TC_STRING, 0), "");
					auto const type = cyng::value_cast<std::uint32_t>(res->get(5, cyng::TC_UINT32, 0), 0);
					auto const obj = cyng::table::restore(val, type);

					//
					//	insert split lines between sections
					//
					auto const opath = sml::translate_obis_path(path);
					auto section_new = get_first_section(opath, ':');
					if (!boost::algorithm::equals(section_new, section)) {
						section = section_new;
						os
							<< std::string(42, '-')
							<< "  ["
							<< section_new
							<< ']'
							<< std::endl;
					}

					os
						<< std::setfill('.') 
						<< std::setw(42)
						<< std::left
						<< opath
						<< ": "
						;
					list_value(os, val, def);
					os
						<< " ("
						<< cyng::io::to_str(obj)
						<< ':'
						<< obj.get_class().type_name()
						<< ")"
						<< std::endl;
				}
			}
		}
		return r.second;
	}

	std::string get_first_section(std::string path, char sep)
	{
		std::string op;
		auto const r = cyng::split(path, std::string(1, sep));
		return (r.empty())
			? path
			: r.at(0)
			;
	}

	bool init_config_record(cyng::db::session& s, std::string const& key, cyng::object obj)
	{
		//
		//	ToDo: use already prepared statements
		//
		cyng::table::meta_table_ptr meta = storage::mm_.at("TCfg");
		cyng::sql::command cmd(meta, s.get_dialect());
		auto const sql = cmd.insert()();
		auto stmt = s.create_statement();
		std::pair<int, bool> r = stmt->prepare(sql);
		if (r.second) {
			//	repackaging as string
			auto const val = cyng::make_object(cyng::io::to_str(obj));
			stmt->push(cyng::make_object(key), 128);	//	pk
			stmt->push(cyng::make_object(1u), 0);	//	gen
			stmt->push(val, 256);	//	val
			stmt->push(val, 256);	//	def
			stmt->push(cyng::make_object(obj.get_class().tag()), 0);	//	type
			if (stmt->execute())	return true;
		}
		return false;
	}

	std::string get_profile_name(std::uint32_t profile)
	{
		switch (profile) {
		case 0x10:	return "1 minute";
		case 0x11:	return "15 minute";
		case 0x12:	return "60 minute";
		case 0x13:	return "24 hour";
		case 0x14:	return "last 2 hours";
		case 0x15:	return "last week";
		case 0x16:	return "1 month";
		case 0x17:	return "1 year";
		case 0x18:	return "initial";
		default:
			break;
		}
		return "";
	}

	std::string get_sql_select_all(std::uint32_t profile)
	{
		switch (profile) {
		case 0x10:	return "SELECT P10.clientID, P10.tsidx, datetime(P10.actTime), status, S10.OBIS, S10.val, S10.type, S10.scaler, S10.unit FROM TProfile_8181C78610FF P10 JOIN TStorage_8181C78610FF S10 ON P10.clientID = S10.clientID AND P10.tsidx = S10.tsidx ORDER BY S10.tsidx;";
		case 0x11:	return "SELECT P11.clientID, P11.tsidx, datetime(P11.actTime), status, S11.OBIS, S11.val, S11.type, S11.scaler, S11.unit FROM TProfile_8181C78611FF P11 JOIN TStorage_8181C78611FF S11 ON P11.clientID = S11.clientID AND P11.tsidx = S11.tsidx ORDER BY S11.tsidx;";
		case 0x12:	return "SELECT P12.clientID, P12.tsidx, datetime(P12.actTime), status, S12.OBIS, S12.val, S12.type, S12.scaler, S12.unit FROM TProfile_8181C78612FF P12 JOIN TStorage_8181C78612FF S12 ON P12.clientID = S12.clientID AND P12.tsidx = S12.tsidx ORDER BY S12.tsidx;";
		case 0x13:	return "SELECT P13.clientID, P13.tsidx, datetime(P13.actTime), status, S13.OBIS, S13.val, S13.type, S13.scaler, S13.unit FROM TProfile_8181C78613FF P13 JOIN TStorage_8181C78613FF S13 ON P13.clientID = S13.clientID AND P13.tsidx = S13.tsidx ORDER BY S13.tsidx;";
		case 0x14:	return "SELECT P14.clientID, P14.tsidx, datetime(P14.actTime), status, S14.OBIS, S14.val, S14.type, S14.scaler, S14.unit FROM TProfile_8181C78614FF P14 JOIN TStorage_8181C78614FF S14 ON P14.clientID = S14.clientID AND P14.tsidx = S14.tsidx ORDER BY S14.tsidx;";
		case 0x15:	return "SELECT P15.clientID, P15.tsidx, datetime(P15.actTime), status, S15.OBIS, S15.val, S15.type, S15.scaler, S15.unit FROM TProfile_8181C78615FF P15 JOIN TStorage_8181C78615FF S15 ON P15.clientID = S15.clientID AND P15.tsidx = S15.tsidx ORDER BY S15.tsidx;";
		case 0x16:	return "SELECT P16.clientID, P16.tsidx, datetime(P16.actTime), status, S16.OBIS, S16.val, S16.type, S16.scaler, S16.unit FROM TProfile_8181C78616FF P16 JOIN TStorage_8181C78616FF S16 ON P16.clientID = S16.clientID AND P16.tsidx = S16.tsidx ORDER BY S16.tsidx;";
		case 0x17:	return "SELECT P17.clientID, P17.tsidx, datetime(P17.actTime), status, S17.OBIS, S17.val, S17.type, S17.scaler, S17.unit FROM TProfile_8181C78617FF P17 JOIN TStorage_8181C78617FF S17 ON P17.clientID = S17.clientID AND P17.tsidx = S17.tsidx ORDER BY S17.tsidx;";
		case 0x18:	return "SELECT P18.clientID, P18.tsidx, datetime(P18.actTime), status, S18.OBIS, S18.val, S18.type, S18.scaler, S18.unit FROM TProfile_8181C78618FF P18 JOIN TStorage_8181C78618FF S18 ON P18.clientID = S18.clientID AND P18.tsidx = S18.tsidx ORDER BY S18.tsidx;";
		default:
			//	unknown profile
			break;
		}
		return "";
	};

	std::string get_sql_select(std::uint32_t profile
		, std::chrono::system_clock::time_point start
		, std::chrono::system_clock::time_point end)
	{
		switch (profile) {
		case 0x10:	return "SELECT P10.clientID, P10.tsidx, datetime(P10.actTime), status, S10.OBIS, S10.val, S10.type, S10.scaler, S10.unit FROM TProfile_8181C78610FF P10 JOIN TStorage_8181C78610FF S10 ON P10.clientID = S10.clientID AND P10.tsidx = S10.tsidx ORDER BY S10.tsidx;";
		case 0x11:	return "SELECT P11.clientID, P11.tsidx, datetime(P11.actTime), status, S11.OBIS, S11.val, S11.type, S11.scaler, S11.unit FROM TProfile_8181C78611FF P11 JOIN TStorage_8181C78611FF S11 ON P11.clientID = S11.clientID AND P11.tsidx = S11.tsidx ORDER BY S11.tsidx;";
		case 0x12:	return "SELECT P12.clientID, P12.tsidx, datetime(P12.actTime), status, S12.OBIS, S12.val, S12.type, S12.scaler, S12.unit FROM TProfile_8181C78612FF P12 JOIN TStorage_8181C78612FF S12 ON P12.clientID = S12.clientID AND P12.tsidx = S12.tsidx ORDER BY S12.tsidx;";
		case 0x13:	return "SELECT P13.clientID, P13.tsidx, datetime(P13.actTime), status, S13.OBIS, S13.val, S13.type, S13.scaler, S13.unit FROM TProfile_8181C78613FF P13 JOIN TStorage_8181C78613FF S13 ON P13.clientID = S13.clientID AND P13.tsidx = S13.tsidx ORDER BY S13.tsidx;";
		case 0x14:	return "SELECT P14.clientID, P14.tsidx, datetime(P14.actTime), status, S14.OBIS, S14.val, S14.type, S14.scaler, S14.unit FROM TProfile_8181C78614FF P14 JOIN TStorage_8181C78614FF S14 ON P14.clientID = S14.clientID AND P14.tsidx = S14.tsidx ORDER BY S14.tsidx;";
		case 0x15:	return "SELECT P15.clientID, P15.tsidx, datetime(P15.actTime), status, S15.OBIS, S15.val, S15.type, S15.scaler, S15.unit FROM TProfile_8181C78615FF P15 JOIN TStorage_8181C78615FF S15 ON P15.clientID = S15.clientID AND P15.tsidx = S15.tsidx ORDER BY S15.tsidx;";
		case 0x16:	return "SELECT P16.clientID, P16.tsidx, datetime(P16.actTime), status, S16.OBIS, S16.val, S16.type, S16.scaler, S16.unit FROM TProfile_8181C78616FF P16 JOIN TStorage_8181C78616FF S16 ON P16.clientID = S16.clientID AND P16.tsidx = S16.tsidx ORDER BY S16.tsidx;";
		case 0x17:	return "SELECT P17.clientID, P17.tsidx, datetime(P17.actTime), status, S17.OBIS, S17.val, S17.type, S17.scaler, S17.unit FROM TProfile_8181C78617FF P17 JOIN TStorage_8181C78617FF S17 ON P17.clientID = S17.clientID AND P17.tsidx = S17.tsidx ORDER BY S17.tsidx;";
		case 0x18:	return "SELECT P18.clientID, P18.tsidx, datetime(P18.actTime), status, S18.OBIS, S18.val, S18.type, S18.scaler, S18.unit FROM TProfile_8181C78618FF P18 JOIN TStorage_8181C78618FF S18 ON P18.clientID = S18.clientID AND P18.tsidx = S18.tsidx ORDER BY S18.tsidx;";
		default:
			//	unknown profile
			break;
		}
		return "";
	}

	std::chrono::system_clock::time_point time_index_to_time_point(std::uint32_t profile, std::uint64_t tsidx)
	{
		switch (profile) {
		case 0x10:	return cyng::chrono::ts_since_passed_minutes(tsidx);
		case 0x11:	return cyng::chrono::ts_since_passed_minutes(tsidx / 15u);
		case 0x12:	return cyng::chrono::ts_since_passed_hours(tsidx);
		case 0x13:	return cyng::chrono::ts_since_passed_days(tsidx);
			//case 14:	return cyng::chrono::ts_since_passed_minutes(profile);
			//case 15:	return cyng::chrono::ts_since_passed_minutes(profile);
		case 0x16:	return cyng::chrono::ts_since_passed_days(tsidx / 30);
		case 0x17:	return cyng::chrono::ts_since_passed_days(tsidx / 365u);
			//case 18:	return cyng::chrono::ts_since_passed_minutes(profile);
		default:
			break;
		}
		return std::chrono::system_clock::now();
	};

}