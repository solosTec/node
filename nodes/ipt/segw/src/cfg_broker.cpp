/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "cfg_broker.h"
#include "segw.h"
#include "cache.h"
#include <smf/sml/obis_db.h>

#include <cyng/value_cast.hpp>

#include <boost/core/ignore_unused.hpp>
#include <boost/predef.h>

namespace node
{
	//
	//	broker configuration
	//
	cfg_broker::cfg_broker(cache& c)
		: cache_(c)
	{}

	bool cfg_broker::is_transparent_mode(std::uint8_t tty) const
	{
		//cache_.get_cfg(build_cfg_key({ OBIS_IF_1107, OBIS_IF_1107_ACTIVE }), false);
		switch (tty) {
		case 0:
			//	ttyAPP0 (wireless mBus)
			//	IF_wMBUS:transparent-mode
			return cache_.get_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "transparent-mode" ), false);

		case 1:
			//	ttyAPP1 (serial Interface)
			//	rs485:transparent-mode
			return cache_.get_cfg(build_cfg_key({ "rs485", "transparent-mode" }), false);
		default:
			break;
		}
		return false;
	}

}
