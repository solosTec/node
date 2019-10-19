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
			db_exporter(cyng::table::meta_map_t const&, std::string const& schema);
			db_exporter(cyng::table::meta_map_t const&, std::string const& schema
				, std::uint32_t source
				, std::uint32_t channel
				, std::string const& target);


			/**
			 * reset exporter
			 */
			void reset();

			bool write_data(cyng::db::session
				, boost::uuids::uuid pk
				, std::size_t idx
				, cyng::buffer_t const& code
				, std::string const& value
				, std::string const& unit
				, std::string const& status);

			bool write_meta(cyng::db::session
				, boost::uuids::uuid pk
				, std::string const& meter
				, std::string const& status
				, bool bcc
				, std::size_t size);

		private:
			cyng::table::meta_map_t const & mt_;
			std::string const schema_;
			std::uint32_t const source_;
			std::uint32_t const channel_;
			std::string const target_;

			boost::uuids::random_generator rgn_;
		};

	}	//	iec
}

#endif
