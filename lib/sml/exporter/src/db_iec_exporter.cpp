/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/exporter/db_iec_exporter.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/units.h>
#include <smf/sml/scaler.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/io/serializer.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/intrinsics/traits/tag_names.hpp>
#include <cyng/sql.h>
#include <cyng/db/interface_statement.h>
#include <cyng/factory.h>

#include <boost/uuid/nil_generator.hpp>

namespace node
{
	namespace iec
	{
		db_exporter::db_exporter(cyng::table::mt_table const& mt, std::string const& schema)
			: mt_(mt)
			, schema_(schema)
			, source_(0)
			, channel_(0)
			, target_()
			, rgn_()
		{
			reset();
		}

		db_exporter::db_exporter(cyng::table::mt_table const& mt
			, std::string const& schema
			, std::uint32_t source
			, std::uint32_t channel
			, std::string const& target)
		: mt_(mt)
			, schema_(schema)
			, source_(source)
			, channel_(channel)
			, target_(target)
			, rgn_()
		{
			reset();
		}


		void db_exporter::reset()
		{
		}

		void db_exporter::write_data(cyng::db::session sp
			, boost::uuids::uuid pk
			, std::size_t idx
			, cyng::buffer_t const& buffer
			, std::string const& value
			, std::string const& unit
			, std::string const& status)
		{

			//{ "pk"		//	join to TIECMeta
			//, "idx"		//	message index
			//, "OBIS"	//	OBIS code
			//, "val"		//	value
			//, "unit"	//	physical unit
			//, "status" },

			cyng::sql::command cmd(mt_.find("TIECData")->second, sp.get_dialect());
			cmd.insert();
			auto sql = cmd.to_str();
			//CYNG_LOG_INFO(logger_, "db.insert.data " << sql);
			auto stmt = sp.create_statement();
			std::pair<int, bool> r = stmt->prepare(sql);
			//BOOST_ASSERT(r.first == 6);	//	11 parameters to bind
			BOOST_ASSERT(r.second);

			//cyng::buffer_t tmp = cyng::value_cast(frame.at(3), tmp);
			node::sml::obis code(buffer);

			stmt->push(cyng::make_object(pk), 36)	//	pk
				.push(cyng::make_object(idx), 0)	//	index
				.push(cyng::make_object(cyng::io::to_hex(code.to_buffer())), 24)	//	OBIS
				.push(cyng::make_object(value), 64)	//	value
				.push(cyng::make_object(unit), 16)	//	unit
				.push(cyng::make_object(status), 24)	//	status
				;


			if (!stmt->execute())
			{
				//CYNG_LOG_ERROR(logger_, sql << " failed with the following data set: " << cyng::io::to_str(frame));

			}
			stmt->clear();

		}

		void db_exporter::write_meta(cyng::db::session sp
			, boost::uuids::uuid pk
			, std::string const& meter
			, std::string const& status
			, bool bcc
			, std::size_t size)
		{
			cyng::sql::command cmd(mt_.find("TIECMeta")->second, sp.get_dialect());
			cmd.insert();
			auto sql = cmd.to_str();
			//CYNG_LOG_INFO(logger_, "db.insert.data " << sql);
			auto stmt = sp.create_statement();
			std::pair<int, bool> r = stmt->prepare(sql);
			//BOOST_ASSERT(r.first == 6);	//	11 parameters to bind
			BOOST_ASSERT(r.second);


			stmt->push(cyng::make_object(pk), 36)		//	pk
				.push(cyng::make_now(), 0)				//	roTime
				.push(cyng::make_object(meter), 8)		//	meter
				.push(cyng::make_object(status), 8)		//	status
				.push(cyng::make_object(bcc), 0)		//	bcc
				.push(cyng::make_object(size), 0)		//	size
				.push(cyng::make_object(source_), 0)	//	source
				.push(cyng::make_object(channel_), 0)	//	channel
				.push(cyng::make_object(target_), 32)	//	target
				;


			if (!stmt->execute())
			{
				//CYNG_LOG_ERROR(logger_, sql << " failed with the following data set: " << cyng::io::to_str(frame));

			}
			stmt->clear();

		}

	}	//	iec
}

