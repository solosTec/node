/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_CONFIG_SCHEMES_H
#define SMF_CONFIG_SCHEMES_H

#include <cyng/store/meta.h>


namespace smf {
	namespace config {

		cyng::meta_store get_store_cluster();
		cyng::meta_sql get_table_cluster();

		cyng::meta_store get_store_device();
		cyng::meta_sql get_table_device();

		cyng::meta_store get_store_lora();
		cyng::meta_sql get_table_lora();

		//
		//	store only schemes
		//
		cyng::meta_store get_store_target();
	}
}

#endif
