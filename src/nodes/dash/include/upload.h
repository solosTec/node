/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_DASH_UPLOAD_H
#define SMF_DASH_UPLOAD_H


#include <cyng/log/logger.h>
#include <boost/uuid/random_generator.hpp>

namespace smf {

	enum class upload_policy {
		APPEND,		//	"append"
		MERGE,		//	"merge"
		OVERWRITE	//	"subst"
	};

	class db;
	class upload
	{

	public:
		upload(cyng::logger, db&);

		void config_bridge(std::string name, std::string policy, std::string const& content, char sep);

	private:
		cyng::logger logger_;
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
