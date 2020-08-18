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
		//	name, cache, custom
		tables::tbl_descr{"TDevice", true, false},
		tables::tbl_descr{"TGateway", true, false},
		tables::tbl_descr{"TLoRaDevice", true, false},
		tables::tbl_descr{"TMeter", true, false},
		tables::tbl_descr{"TLL", false, false},
		tables::tbl_descr{"TGUIUser", true, false},
		tables::tbl_descr{"TNodeNames", true, false},
		tables::tbl_descr{"TIECBridge", true, false},	//	TIECBroker
		tables::tbl_descr{"TGWSnapshot", false, false},
		tables::tbl_descr{"TLoraUplink", false, true}	//	custom handling
	};

	tables::array_t::const_iterator tables::find(std::string name)
	{
		return std::find_if(std::begin(list_), std::end(list_), [&name](tbl_descr const& desc) {
			return boost::algorithm::equals(desc.name_, name);
			});
	}

	bool tables::is_custom(std::string name)
	{
		auto pos = find(name);

		return (pos != std::end(list_))
			? pos->custom_
			: false
			;
	}


}
