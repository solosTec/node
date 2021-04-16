/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_DASH_UPLOAD_H
#define SMF_DASH_UPLOAD_H

#include <smf/cluster/bus.h>

#include <cyng/log/logger.h>

#include <boost/uuid/random_generator.hpp>

namespace smf {

	enum class upload_policy {
		APPEND,		//	"append"
		MERGE,		//	"merge"
		OVERWRITE	//	"subst"
	};

	upload_policy to_upload_policy(std::string);

	class db;
	class upload
	{

	public:
		upload(cyng::logger, bus&, db&);

		void config_bridge(std::string name, upload_policy policy, std::string const& content, char sep);

	private:
		void insert_iec(std::string const& mc
			, std::string const& server_id
			, std::string const& meter_id
			, std::string const& address
			, std::uint16_t port
			, std::string const& meter_type
			, std::string const& area
			, std::string const& name
			, std::string const& maker);
		void insert_wmbus(std::string const& mc
			, std::string const& server_id
			, std::string const& meter_id
			, std::string const& meter_type
			, cyng::crypto::aes_128_key const& aes
			, std::string const& area
			, std::string const& name
			, std::string const& maker);
		void update_iec(cyng::key_t
			, std::string const& mc
			, std::string const& server_id
			, std::string const& meter_id
			, std::string const& address
			, std::uint16_t port
			, std::string const& meter_type
			, std::string const& area
			, std::string const& name
			, std::string const& maker);
		void update_wmbus(cyng::key_t
			, std::string const& mc
			, std::string const& server_id
			, std::string const& meter_id
			, std::string const& meter_type
			, cyng::crypto::aes_128_key const& aes
			, std::string const& area
			, std::string const& name
			, std::string const& maker);

	private:
		cyng::logger logger_;
		bus& cluster_bus_;
		db& db_;

		/**
		 * generate random tags
		 */
		boost::uuids::random_generator uidgen_;

	};

	std::string cleanup_manufacturer_code(std::string manufacturer);
	std::string cleanup_server_id(std::string const& meter_id
		, std::string const& manufacturer_code
		, bool wired
		, std::uint8_t version
		, std::uint8_t medium);

}

#endif
