/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher
 *
 */ 

#include <smf/shared/protocols.h>
#include <boost/algorithm/string.hpp>

namespace node 
{
	std::string to_str(protocol_e prot)
	{
		switch (prot) {
		case protocol_e::RAW:	return protocol_RAW;
		case protocol_e::TCP:	return protocol_TCP;
		case protocol_e::IPT:	return protocol_IPT;
		case protocol_e::IEC:	return protocol_IEC;
		case protocol_e::MBUS:	return protocol_MBUS;
		case protocol_e::wMBUS:	return protocol_wMBUS;
		case protocol_e::HDLC:	return protocol_HDLC;
		case protocol_e::SML:	return protocol_SML;
		case protocol_e::COSEM:	return protocol_COSEM;
		default:
			break;
		}
		return protocol_any;
	}

	protocol_e from_str(std::string const& str)
	{
		//
		//	add more variations id required
		//

		if (boost::algorithm::equals(str, protocol_RAW))	return protocol_e::RAW;
		else if (boost::algorithm::equals(str, protocol_TCP))	return protocol_e::TCP;
		else if (boost::algorithm::equals(str, protocol_IPT))	return protocol_e::IPT;
		else if (boost::algorithm::equals(str, protocol_IEC)
			|| boost::algorithm::starts_with(str, "IEC 62056"))	return protocol_e::IEC;
		else if (boost::algorithm::equals(str, protocol_MBUS))	return protocol_e::MBUS;
		else if (boost::algorithm::equals(str, protocol_wMBUS)
			|| boost::algorithm::starts_with(str, "Wireless MBUS"))	return protocol_e::wMBUS;
		else if (boost::algorithm::equals(str, protocol_HDLC))	return protocol_e::HDLC;
		else if (boost::algorithm::equals(str, protocol_SML))	return protocol_e::SML;
		else if (boost::algorithm::equals(str, protocol_COSEM))	return protocol_e::COSEM;

		return protocol_e::ANY;
	}
}

