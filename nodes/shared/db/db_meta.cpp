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
				{ "pk", "OBIS", "unitCode", "unitName", "dataType", "scaler", "val", "result" },
				{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT8, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_INT32, cyng::TC_INT64, cyng::TC_STRING },
				{ 36, 24, 0, 64, 16, 0, 0, 512 });
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
				{ "serverID"	//	server ID
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
				, "aes"		//	AES-Schlüssel: 
				, "user"
				, "pwd"
				},
				{ cyng::TC_BUFFER		//	server ID
				, cyng::TC_TIME_POINT	//	last seen
				, cyng::TC_STRING		//	device class
				, cyng::TC_BOOL			//	active
				, cyng::TC_STRING		//	description

				, cyng::TC_UINT64		//	status (81 00 60 05 00 00)
				, cyng::TC_BUFFER		//	bit mask (81 81 C7 86 01 FF)
				, cyng::TC_UINT32		//	interval (milliseconds)
				, cyng::TC_BUFFER		//	pubKey
				, cyng::TC_AES128		//	AES 128 (16 bytes)
				, cyng::TC_STRING		//	user
				, cyng::TC_STRING		//	pwd
				},
				{ 9
				, 0
				, 16	//	device class
				, 0		//	active
				, 128	//	description

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
			return cyng::table::make_meta_table<2, 6>(name,
				{ "serverID"	//	server ID
				, "idx"			//	index
				//	-- body
				, "interval"	//	seconds
				, "delay"		//	seconds
				, "target"		//	target name
				, "source"		//	push source (profile, installation parameters, list of visible sensors/actors)
				, "profile"		//	"Lastgang"
				, "task"		//	associated task id

				},
				{ cyng::TC_BUFFER		//	server ID
				, cyng::TC_UINT8		//	index
										//	-- body
				, cyng::TC_UINT32		//	interval [seconds]
				, cyng::TC_UINT32		//	delay [seconds]
				, cyng::TC_STRING		//	target
				, cyng::TC_UINT8		//	source
				, cyng::TC_UINT8		//	profile
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



