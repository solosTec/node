/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_PROTOCOLS_H
#define NODE_PROTOCOLS_H

#include <string>
#include <cyng/compatibility/general.h>

namespace node
{
	enum class protocol_e
	{
		ANY,	//	also unknown
		RAW,
		TCP,
		IPT,	//	DIN-E 43863-4
		IEC,	//	IEC:62056
		MBUS,
		wMBUS,	//	wireless (OMS) - EN13757-4
		HDLC,
		SML,
		COSEM,	//	DLMS/COSEM - Companion Specification for Energy Metering
	};

	std::string to_str(protocol_e);
	protocol_e from_str(std::string const&);

	//	transport
	constexpr char  protocol_any[]		= "any";
	constexpr char  protocol_RAW[]		= "raw";
	constexpr char  protocol_TCP[]		= "tcp";
	constexpr char  protocol_IPT[]		= "IP-T:DIN-E43863-4";
	constexpr char  protocol_HDLC[]		= "HDLC";

	//	data
	constexpr char  protocol_IEC[]		= "IEC:62056";	//	IEC-62056-21:2002
	constexpr char  protocol_MBUS[]		= "M-Bus";
	constexpr char  protocol_wMBUS[]	= "wM-Bus:EN13757-4";
	constexpr char  protocol_SML[]		= "SML";
	constexpr char  protocol_COSEM[]	= "COSEM";

	
}


#endif
