/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_MASTER_CONFIG_CACHE_H
#define NODE_IPT_MASTER_CONFIG_CACHE_H

#include <smf/sml/intrinsics/obis_factory.hpp>


namespace node 
{
	/**
	 * Cache for gateway configuration.
	 * This class makes gateway configuration available also when the device
	 * is offline. In addition, it allows the incremental capturing of all 
	 * configuration data and allows a backup to be created.
	 *
	 * Each instance of this class stores the configuration of one gateway.
	 */
	class config_cache
	{
		using sections_t = std::map<sml::obis_path, cyng::param_map_t>;

	public:
		using device_map_t = std::map<sml::obis, cyng::buffer_t>;

		/**
		 * Parameters are role (1..8), user (1..0xFE), user, device list
		 */
		using cb_access_rights = std::function<void(std::uint8_t, std::uint8_t, std::string, device_map_t)>;

	public:
		/**
		 * Take each section of the obis path as a full obis path itself
		 */
		config_cache(cyng::buffer_t srv, sml::obis_path&&);

		/**
		 * update configuration cache
		 *
		 * @return true if root is part of the actice sections
		 */
		bool update(sml::obis root, cyng::param_map_t const& params);
		bool update(sml::obis_path root, cyng::param_map_t const& params, bool force);

		/**
		 * @return server as string
		 */
		std::string get_server() const;

		/**
		 * @return true if OBIS code is a cached root path.
		 */
		bool is_cached(sml::obis code) const;
		bool is_cached(sml::obis_path const&) const;

		/**
		 * extend cache of specified root slots if required
		 */
		void add(sml::obis_path&&);
		void remove(sml::obis_path&&);

		/**
		 * remove all root slots
		 */
		void clear();

		/**
		 * @return data of the cached section. Data set will be empty
		 * if no data were cached.
		 */
		cyng::param_map_t get_section(sml::obis_path const&) const;
		cyng::param_map_t get_section(sml::obis) const;

		/**
		 * Loop over access rights (81 81 81 60 FF FF)
		 */
		void loop_access_rights(cb_access_rights) const;

	private:
		/**
		 * Take each section of the obis path as a full obis path itself
		 */
		static sections_t init_sections(sml::obis_path&&);

	private:
		cyng::buffer_t const srv_;
		sections_t sections_;
	};


	/**
	 * filtering devices from list of access rights
	 */
	config_cache::device_map_t split_list_of_access_rights(cyng::param_map_t& params);

	/**
	 * @return value 81818161FFFF
	 */
	std::string get_user_name(cyng::param_map_t const& params);
}


#endif

