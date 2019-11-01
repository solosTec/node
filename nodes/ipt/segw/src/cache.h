/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_SEGW_CACHE_H
#define NODE_IPT_SEGW_CACHE_H

#include <smf/sml/status.h>
#include <smf/ipt/config.h>

#include <cyng/store/db.h>
#include <cyng/io/serializer.h>

#include <boost/uuid/uuid.hpp>
#include <boost/algorithm/string.hpp>

namespace node
{

	/**
	 * create a map with all table meta data.
	 */
	cyng::table::meta_vec_t create_cache_meta_data();

	/**
	 * create all required cache tables
	 */
	void init_cache(cyng::store::db&);

	/**
	 * manage cached tables
	 */
	class bridge;
	class cache
	{
		friend class bridge;
		friend void init_cache(cyng::store::db&);

	public:
		cache(cyng::store::db&, boost::uuids::uuid tag);

		/**
		 * @return itentity/source tag
		 */
		boost::uuids::uuid const get_tag() const;

		/**
		 * atomic change of status word
		 */
		void set_status_word(sml::status_bits, bool);

		/**
		 * read status word from "_Cfg" cache 
		 */
		std::uint64_t get_status_word();

		/**
		 * read a configuration value from table "_Cfg"
		 */
		template <typename T >
		T get_cfg(std::string name, T def) {
			return cyng::value_cast(db_.get_value("_Cfg", std::string("val"), name), def);
		}

		/**
		 * set/insert a configuration value
		 */
		template <typename T >
		bool set_cfg(std::string name, T val) {
			return merge_cfg(name, cyng::make_object(val));
		}

		/**
		 * set/insert a configuration value
		 */
		bool merge_cfg(std::string name, cyng::object&& obj);

		void read_table(std::string const&, std::function<void(cyng::store::table const*)>);
		void read_tables(std::string const&, std::string const&, std::function<void(cyng::store::table const*, cyng::store::table const*)>);
		void write_table(std::string const&, std::function<void(cyng::store::table*)>);

		/**
		 * Convinience function to loop over one table with read access
		 */
		void loop(std::string const&, std::function<bool(cyng::table::record const&)>);


		/** @brief 81 48 27 32 06 01
		 *
		 * @return wait before TCP/IP reconnect (minutes)
		 */
		std::chrono::minutes get_ipt_tcp_wait_to_reconnect();

		/** @brief 81 48 31 32 02 01
		 *
		 * A value of 0 implies that no attempts are made to initiate a connection.
		 *
		 * @return max. number of retries to reconnect.
		 */
		std::uint32_t get_ipt_tcp_connect_retries();

		/** @brief 00 80 80 00 03 01
		 *
		 * @return true if SSL is configured
		 */
		bool has_ipt_ssl();

		/**
		 * extract IP-T configuration
		 */
		ipt::redundancy get_ipt_redundancy();
		ipt::master_record get_ipt_master();

		/**
		 * get current IP-T master
		 */
		std::uint8_t get_ipt_master_index();

		/**
		 * get current IP-T scramble key
		 */
		ipt::scramble_key get_ipt_sk();

		/**
		 * Switch to other IP-T redundancy
		 * Currently only 2 redundancies supported, so the only viable
		 * strategy is a round-robin algorithm.
		 *
		 * @return new master config record
		 */
		ipt::master_record switch_ipt_redundancy();

	private:
		/**
		 * build up meta data
		 */
		static cyng::table::meta_map_t get_meta_map();

		/**
		 * Insert/update a configuration value
		 */
		template <typename T >
		bool set_config_value(cyng::store::table* tbl, std::string name, T val) {

			if (boost::algorithm::equals(tbl->meta().get_name(), "_Cfg")) {
				return tbl->merge(cyng::table::key_generator(name)
					, cyng::table::data_generator(std::move(val))
					, 1u	//	only needed for insert operations
					, tag_);
			}
			return false;
		}

		/**
		 * extract IP-T host configuration
		 * @param idx only the values 1 and 2 are valid
		 */
		std::string get_ipt_host(std::uint8_t idx);
		std::uint16_t get_ipt_port_target(std::uint8_t idx);
		std::uint16_t get_ipt_port_source(std::uint8_t idx);
		std::string get_ipt_user(std::uint8_t idx);
		std::string get_ipt_pwd(std::uint8_t idx);
		bool is_ipt_scrambled(std::uint8_t idx);
		ipt::scramble_key get_ipt_sk(std::uint8_t idx);

		/**
		 * extract IP-T configuration
		 * @param idx only the values 1 and 2 are valid
		 */
		ipt::master_record get_ipt_cfg(std::uint8_t idx);

	private:
		static cyng::table::meta_map_t const mm_;

		/**
		 * global data cache
		 */
		cyng::store::db& db_;

		/**
		 * source tag
		 */
		boost::uuids::uuid const tag_;
	};

	/**
	 * @return the specified configuration object
	 */
	cyng::object get_config_obj(cyng::store::table const* tbl, std::string name);

	/**
	 * @return the specified configuration value
	 */
	template <typename T >
	T get_config_value(cyng::store::table const* tbl, std::string name, T def) {
		return cyng::value_cast<T>(get_config_obj(tbl, name), def);
	}


}
#endif
