/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_EXPORTER_DB_IEC_H
#define NODE_SML_EXPORTER_DB_IEC_H


#include <smf/sml/defs.h>
#include <smf/sml/intrinsics/obis.h>
#include <smf/sml/units.h>
#include <cyng/db/session.h>
#include <cyng/table/meta_interface.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/object.h>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	namespace iec
	{
		/**
		 * Write IEC data into SQL database.
		 */
		class db_exporter
		{
		public:
			db_exporter(cyng::table::mt_table const&, std::string const& schema);
			db_exporter(cyng::table::mt_table const&, std::string const& schema
				, std::uint32_t source
				, std::uint32_t channel
				, std::string const& target);


			/**
			 * reset exporter
			 */
			void reset();

			void write_data(cyng::db::session
				, boost::uuids::uuid pk
				, std::size_t idx
				, cyng::buffer_t const& code
				, std::string const& value
				, std::string const& unit
				, std::string const& status);

			void write_meta(cyng::db::session
				, boost::uuids::uuid pk
				, std::string const& meter
				, std::string const& status
				, bool bcc
				, std::size_t size);

		private:

			//
			//	database functions
			//
			//void store_meta(cyng::db::session sp
			//	, boost::uuids::uuid pk
			//	, std::string const& trx
			//	, std::size_t idx
			//	, cyng::object ro_ime
			//	, cyng::object act_ime
			//	, cyng::object val_time
			//	, cyng::object client		//	gateway
			//	, cyng::object client_id	//	gateway - formatted
			//	, cyng::object server		//	server
			//	, cyng::object server_id	//	server - formatted
			//	, cyng::object status
			//	, obis profile);

			//void store_data(cyng::db::session sp
			//	, boost::uuids::uuid pk
			//	, std::string const& trx
			//	, std::size_t idx
			//	, obis const& code
			//	, std::uint8_t unit
			//	, std::string unit_name
			//	, std::string type_name	//	CYNG data type name
			//	, std::int8_t scaler	//	scaler
			//	, cyng::object raw		//	raw value
			//	, cyng::object value);

		private:
			const cyng::table::mt_table& mt_;
			const std::string schema_;
			const std::uint32_t source_;
			const std::uint32_t channel_;
			const std::string target_;

			boost::uuids::random_generator rgn_;
		};

	}	//	iec
}

#endif
