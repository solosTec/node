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


		/**
		 * device/TDevice
		 */
		cyng::meta_store get_store_device();
		cyng::meta_sql get_table_device();

		/**
		 * loRaDevice/TLoRaDevice
		 */
		cyng::meta_store get_store_lora();
		cyng::meta_sql get_table_lora();

		/**
		 * guiUser/TGuiUser
		 */
		cyng::meta_store get_store_gui_user();
		cyng::meta_sql get_table_gui_user();

		//
		//	store only schemes
		//
		cyng::meta_store get_store_target();

		/**
		 * cluster/TCluster
		 */
		cyng::meta_store get_store_cluster();

		/**
		 * container to store meta data of in-memory tables
		 */
		using store_map = std::map<std::string, cyng::meta_store>;

		/**
		 * container to store meta data of SQL tables
		 */
		using sql_map = std::map<std::string, cyng::meta_sql>;
	}
}

#endif
