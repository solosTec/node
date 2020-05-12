/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "tables.h"
#include <boost/algorithm/string.hpp>

namespace node
{
	/**
	 * Initialize all used table names
	 */
	const tables::array_t	tables::list_ =
	{
		tables::tbl_descr{"TDevice", false, false},
		tables::tbl_descr{"TGateway", true, false},	//	custom
		tables::tbl_descr{"TLoRaDevice", false, false},
		tables::tbl_descr{"TMeter", true, false},		//	custom
		tables::tbl_descr{"TGUIUser", false, false},
		tables::tbl_descr{"TGWSnapshot", false, false},
		tables::tbl_descr{"TNodeNames", false, false},
		tables::tbl_descr{"TIECBridge", true, false},	//	custom
		tables::tbl_descr{"_Session", false, false},
		tables::tbl_descr{"_Target", false, false},
		tables::tbl_descr{"_Connection", false, false},
		tables::tbl_descr{"_Cluster", false, false},
		tables::tbl_descr{"_Config", false, false},
		tables::tbl_descr{"_SysMsg", false, false},
		tables::tbl_descr{"_TimeSeries", false, false},
		tables::tbl_descr{"_LoRaUplink", false, false},
		tables::tbl_descr{"_CSV", false, false},
		tables::tbl_descr{"_HTTPSession", false, true}	//	local handling
	};

	tables::array_t::const_iterator tables::find(std::string name)
	{
		return std::find_if(std::begin(list_), std::end(list_), [&name](tbl_descr const& desc) {
			return boost::algorithm::equals(desc.name_, name);
			});
	}

	bool tables::is_custom(std::string name)
	{
		auto const pos = find(name);

		return (pos != std::end(list_))
			? pos->custom_
			: false
			;
	}

	bool tables::is_local(std::string name)
	{
		auto const pos = find(name);

		return (pos != std::end(list_))
			? pos->local_
			: false
			;
	}


}
