/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_DB_PROCESSOR_H
#define NODE_IPT_STORE_DB_PROCESSOR_H

#include <smf/sml/protocol/parser.h>
#include <smf/sml/exporter/db_exporter.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/vm/controller.h>
#include <cyng/db/session.h>
#include <cyng/table/meta_interface.h>

namespace node
{
	class storage_db;
	class db_processor
	{
		friend storage_db;

	public:
		db_processor(cyng::io_service_t& ios
			, cyng::logging::log_ptr
			, boost::uuids::uuid tag
			, std::uint32_t channel
			, std::uint32_t source
			, std::string target
			, cyng::db::session
			, cyng::table::mt_table&);

		virtual ~db_processor();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * push data
		 */
		void process(cyng::buffer_t const&);

	private:
		void init();
		void sml_msg(cyng::context& ctx);
		void sml_eom(cyng::context& ctx);
		void insert_meta_data(cyng::context& ctx);

	private:
		cyng::logging::log_ptr logger_;
		cyng::controller vm_;
		sml::parser parser_;
		cyng::db::session db_;
		cyng::table::mt_table mt_table_;
		sml::db_exporter exporter_;	
	};
}

#endif