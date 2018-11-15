/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */


#include "db_meta.h"
#include <cyng/table/meta.hpp>

namespace node
{
	cyng::table::meta_table_ptr TSMLMeta()
	{
		return cyng::table::make_meta_table<1, 12>("TSMLMeta",
			{ "pk", "trxID", "msgIdx", "roTime", "actTime", "valTime", "gateway", "server", "status", "source", "channel", "target", "profile" },
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_TIME_POINT, cyng::TC_TIME_POINT, cyng::TC_UINT32, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_UINT32, cyng::TC_UINT32, cyng::TC_UINT32, cyng::TC_STRING, cyng::TC_STRING },
			{ 36, 16, 0, 0, 0, 0, 23, 23, 0, 0, 0, 32, 24 });
	}

	cyng::table::meta_table_ptr TSMLData()
	{
		return cyng::table::make_meta_table<2, 6>("TSMLData",
			{ "pk", "OBIS", "unitCode", "unitName", "dataType", "scaler", "val", "result" },
			{ cyng::TC_UUID, cyng::TC_STRING, cyng::TC_UINT8, cyng::TC_STRING, cyng::TC_STRING, cyng::TC_INT32, cyng::TC_INT64, cyng::TC_STRING },
			{ 36, 24, 0, 64, 16, 0, 0, 512 });
	}

}



