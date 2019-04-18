/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */


#include "db_meta.h"
#include <cyng/table/meta.hpp>
#include <cyng/store/db.h>
#include <boost/algorithm/string.hpp>

namespace node
{
	cyng::table::meta_table_ptr create_meta(std::string name)
	{
		if (boost::algorithm::equals(name, "TSMLMeta")) {

			return cyng::table::make_meta_table<1, 12>(name,
				{ "pk", "trxID", "msgIdx", "roTime", "actTime", "valTime", "gateway", "server", "status", "source", "channel", "target", "profile" },
				{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_TIME_POINT, cyng::TC_TIME_POINT, cyng::TC_UINT32, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_UINT32, cyng::TC_UINT32, cyng::TC_STRING, cyng::TC_STRING },
				{ 36, 16, 0, 0, 0, 0, 23, 23, 0, 0, 0, 32, 24 });
		}
		else if (boost::algorithm::equals(name, "TSMLData")) {

			return cyng::table::make_meta_table<2, 6>(name,
				{ "pk"
				, "OBIS"
				, "unitCode"
				, "unitName"
				, "dataType"
				, "scaler"
				, "val"
				, "result" //	processed value
				},
				{ cyng::TC_UUID		//	pk
				, cyng::TC_STRING	//	OBIS
				, cyng::TC_UINT8	//	unitCode
				, cyng::TC_STRING	//	unitName
				, cyng::TC_STRING	//	dataType
				, cyng::TC_INT32	//	scaler
				, cyng::TC_INT64	//	val
				, cyng::TC_STRING },//	result
				{ 36
				, 24
				, 0
				, 64
				, 16
				, 0
				, 0
				, 512 });
		}
		else if (boost::algorithm::equals(name, "TIECMeta")) {

			//
			//	trxID - unique for every message
			//	msgIdx - message index
			//	status - M-Bus status
			//
			return cyng::table::make_meta_table<1, 8>(name,
				{ "pk"
				, "roTime"
				, "meter"
				, "status"
				, "bcc"
				, "size"
				, "source"
				, "channel"
				, "target"
				},
				{ cyng::TC_UUID	//	pk
				, cyng::TC_TIME_POINT	//	roTime
				, cyng::TC_STRING	//	meter
				, cyng::TC_STRING	//	status
				, cyng::TC_BOOL		//	bcc
				, cyng::TC_UINT64	//	size
				, cyng::TC_UINT32	//	source
				, cyng::TC_UINT32	//	channel
				, cyng::TC_STRING	//	target
				},
				{ 36	//	pk
				, 0		//	roTime
				, 8		//	meter
				, 8		//	status
				, 0		//	bcc
				, 0		//	size
				, 0		//	source
				, 0		//	channel
				, 32	//	target
				});
		}
		else if (boost::algorithm::equals(name, "TIECData")) {

			//
			//	unitCode - physical unit
			//	unitName - descriptiv
			//	
			return cyng::table::make_meta_table<2, 4>(name,
				{ "pk"		//	join to TIECMeta
				, "idx"		//	message index
				, "OBIS"	//	OBIS code
				, "val"		//	value
				, "unit"	//	physical unit
				, "status" },
				{ cyng::TC_UUID			//	pk
				, cyng::TC_UINT32		//	idx
				, cyng::TC_STRING		//	OBIS
				, cyng::TC_STRING		//	val
				, cyng::TC_STRING		//	unir
				, cyng::TC_STRING },	//	status
				{ 36	//	pk
				, 0		//	idx
				, 24	//	OBIS
				, 64	//	val
				, 16	//	unit
				, 24	//	status
				});
		}
		else if (boost::algorithm::equals(name, "mbus-devices")) {
			return cyng::table::make_meta_table<1, 11>(name,
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
				, "aes"		//	AES-Schl�ssel: 
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
				});
		}
		else if (boost::algorithm::equals(name, "iec62056-21-devices")) {

			//
			//	ToDo: use an u8 data type as index and maintain the index to avoid
			//	gaps between the indexes and to start 0.
			//
			return cyng::table::make_meta_table<1, 5>(name,
				{ "meterID"		//	max. 32 bytes (8181C7930AFF)
				, "address"		//	mostly the same as meterID (8181C7930CFF)
				, "descr"
				, "baudrate"	//	9600, ... (in opening sequence) (8181C7930BFF)
				, "p1"			//	login password (8181C7930DFF)
				//, "p2"		//	login password
				, "w5"			//	W5 password (reserved for national applications) (8181C7930EFF)
				},
				{ cyng::TC_BUFFER		//	meterID (max. 32)
				, cyng::TC_BUFFER		//	address
				, cyng::TC_STRING		//	description
				, cyng::TC_UINT32		//	speed
				, cyng::TC_STRING		//	pwd
				, cyng::TC_STRING		//	w5
				},
				{ 32	//	meterID
				, 32	//	address
				, 128	//	description
				, 0		//	speed
				, 32	//	pwd
				, 32	//	w5
				});
		}
		else if (boost::algorithm::equals(name, "push.ops")) {

			//
			//	Definition of all configured push targets
			//
			return cyng::table::make_meta_table<2, 6>(name,
				{ "serverID"	//	server ID
				, "idx"			//	index
				//	-- body
				, "interval"	//	seconds
				, "delay"		//	seconds
				, "target"		//	target name
				, "source"		//	push source - OBIS_PUSH_SOURCE (profile, installation parameters, list of visible sensors/actors)
				, "profile"		//	"Lastgang" - OBIS_PROFILE
				, "task"		//	associated task id

				},
				{ cyng::TC_BUFFER		//	server ID
				, cyng::TC_UINT8		//	index
										//	-- body
				, cyng::TC_UINT32		//	interval [seconds]
				, cyng::TC_UINT32		//	delay [seconds]
				, cyng::TC_STRING		//	target
				, cyng::TC_BUFFER		//	source
				, cyng::TC_BUFFER		//	profile => reference to "data.collector"
				, cyng::TC_UINT64		//	task

				},
				{ 9		//	server ID
				, 0		//	index
						//	-- body
				, 0		//	interval [seconds]
				, 0		//	delay
				, 64	//	target
				, 0		//	source
				, 0		//	profile
				, 0		//	task id
				});
		}
		else if (boost::algorithm::equals(name, "op.log")) {

			return cyng::table::make_meta_table<1, 10>(name,
				{ "idx"			//	index
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
				});
		}
		else if (boost::algorithm::equals(name, "_Config")) {

			return cyng::table::make_meta_table<1, 1>(name, { "name"	//	parameter name
				, "value"	//	parameter value
				},
				{ cyng::TC_STRING, cyng::TC_STRING },
				{ 64, 128 });
		}
		else if (boost::algorithm::equals(name, "readout")) {
			
			//
			//	last record of a sensor/meter
			//
			return cyng::table::make_meta_table<2, 6>(name,
				{ "serverID"	//	server/meter/sensor ID
				, "OBIS"		//	OBIS code
								//	-- body
				, "unitCode"	//	M-Bus
				, "scaler"		//	signed integer
				, "val"			//	original value
				, "result"		//	processed value
				, "status"		//	status
				, "roTime"		//	timepoint (UTC)
				},
				{ cyng::TC_BUFFER		//	serverID
				, cyng::TC_STRING		//	OBIS
										//	-- body
				, cyng::TC_UINT8		//	unitCode
				, cyng::TC_INT8			//	scaler
				, cyng::TC_INT64		//	val
				, cyng::TC_STRING		//	result
				, cyng::TC_UINT32		//	status
				, cyng::TC_TIME_POINT	//	roTime
				},
				{ 9		//	serverID
				, 24	//	OBIS
						//	-- body
				, 0		//	unitCode
				, 0		//	scaler
				, 0		//	val
				, 512	//	result
				, 0		//	status
				, 23	//	roTime
				});
		}
		else if (boost::algorithm::equals(name, "data.collector")) {

			//
			//	Defines how the data is to be stored. 
			//	81 81 C7 86 20 FF - OBIS_CODE_ROOT_DATA_COLLECTOR
			//
			return cyng::table::make_meta_table<2, 4>(name,
				{ "serverID"	//	server/meter/sensor ID
				, "nr"			//	position/number
								//	-- body
				, "profile"		//	OBIS code
				, "active"		//	turned on/off
				, "maxSize"		//	signed integer
				, "period"		//	register period
				},
				{ cyng::TC_BUFFER		//	serverID
				, cyng::TC_UINT8		//	nr
										//	-- body
				, cyng::TC_BUFFER		//	profile
				, cyng::TC_BOOL			//	active
				, cyng::TC_UINT16		//	maxSize
				, cyng::TC_SECOND		//	period
				},
				{ 9		//	serverID
				, 0		//	nr
						//	-- body
				, 24	//	profile
				, 0		//	active
				, 0		//	maxSize
				, 0		//	period
				});
		}
		else if (boost::algorithm::equals(name, "trx")) {

			//
			//	Transaction ID
			//	example: "19041816034914837-2"
			//	Server ID
			//	example:  01 EC 4D 01 00 00 10 3C 02
			//
			return cyng::table::make_meta_table<1, 3>(name,
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
				, 32		//	msg
				});
		}

		//
		//	table name not defined
		//
		return cyng::table::meta_table_ptr();
	}

	bool create_table(cyng::store::db& db, std::string name)
	{
		return db.create_table(create_meta(name));
	}
}



