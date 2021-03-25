/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MAIN_DB_H
#define SMF_MAIN_DB_H

#include <smf/config/kv_store.h>

#include <smf/config/schemes.h>

#include <cyng/log/logger.h>

#include <boost/uuid/random_generator.hpp>

namespace smf {

	class db
	{

	public:
		db(cyng::store& cache, cyng::logger, boost::uuids::uuid tag);

		inline kv_store& get_cfg() {
			return cfg_;
		}
		inline kv_store const& get_cfg() const {
			return cfg_;
		}

		/**
		 * fill store map and create all tables
		 */
		void init(cyng::param_map_t const&);

		/**
		 * Add a new cluster node into table "cluster"
		 */
		bool insert_cluster_member(boost::uuids::uuid
			, std::string class_name
			, cyng::version
			, boost::asio::ip::tcp::endpoint
			, cyng::pid);

		bool remove_cluster_member(boost::uuids::uuid);

		bool insert_pty(boost::uuids::uuid tag
			, boost::uuids::uuid peer
			, boost::uuids::uuid dev
			, std::string const& name
			, std::string const& pwd
			, boost::asio::ip::tcp::endpoint ep
			, std::string const& data_layer);

		bool remove_pty(boost::uuids::uuid);

		/**
		 * remove all sessions of this peer. "cluster" table
		 * will be updated too.
		 * 
		 * @return number of removed sessions
		 */
		std::size_t remove_pty_by_peer(boost::uuids::uuid);

		/**
		 * update pty counter in cluster table according to current 
		 * count of members in "session" table
		 */
		std::size_t update_pty_counter(boost::uuids::uuid peer);

		/**
		 * @return the tag of the device with the specified credentials. Returns a null-tag
		 * if no matching device was found. The boolean signals if the device is enabled or not
		 */
		std::pair<boost::uuids::uuid, bool> 
			lookup_device(std::string const& name, std::string const& pwd);

		bool insert_device(boost::uuids::uuid tag
			, std::string const& name
			, std::string const& pwd
			, bool enabled);

	private:
		void set_start_values(cyng::param_map_t const& session_cfg);

	private:
		cyng::store& cache_;
		cyng::logger logger_;
		kv_store cfg_;
		config::store_map store_map_;
		boost::uuids::random_generator uuid_gen_;
		std::uint32_t source_;
	};

	/**
	 * @return vector of all required meta data
	 */
	std::vector< cyng::meta_store > get_store_meta_data();

}

#endif
