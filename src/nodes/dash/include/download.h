/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_DASH_DOWNLOAD_H
#define SMF_DASH_DOWNLOAD_H

#include <smf/cluster/bus.h>

#include <cyng/log/logger.h>

#include <filesystem>

namespace smf {

	class db;
	class download
	{

	public:
		download(cyng::logger, bus&, db&);

		std::filesystem::path generate(std::string table, std::string format);

	private:

	private:
		cyng::logger logger_;
		bus& cluster_bus_;
		db& db_;

	};

}

#endif
