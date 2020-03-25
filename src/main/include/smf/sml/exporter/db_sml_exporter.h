/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_EXPORTER_DB_H
#define NODE_SML_EXPORTER_DB_H

#include <smf/sml/defs.h>
#include <smf/sml/units.h>
#include <smf/sml/protocol/readout.h>
#include <cyng/db/session.h>
#include <cyng/table/meta_interface.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/object.h>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	namespace sml
	{
		/**
		 * walk down SML message body recursively, collect data
		 * and write data into SQL database.
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

			void write(cyng::db::session, cyng::tuple_t const&);

		private:
			/**
			 * read SML message.
			 */
			void read_body(cyng::db::session, std::uint16_t code, cyng::tuple_t const& tpl);
			bool read_get_profile_list_response(cyng::db::session, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_get_proc_parameter_response(cyng::db::session, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_attention_response(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			void read_time(std::string const&, cyng::object);
			std::vector<obis> read_param_tree_path(cyng::object);
			obis read_obis(cyng::object);
			std::uint8_t read_unit(std::string const&, cyng::object);
			std::string read_string(std::string const&, cyng::object);

			/**
			 * @return CYNG data type tag
			 */
			std::size_t read_value(obis, std::int8_t, std::uint8_t, cyng::object);
			void read_parameter(obis, cyng::object);
			std::string read_server_id(cyng::object);
			std::string read_client_id(cyng::object);

			void read_period_list(cyng::db::session, std::vector<obis> const&, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			bool read_period_entry(cyng::db::session, std::vector<obis> const&, std::size_t, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_param_tree(std::size_t, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			//
			//	database functions
			//
			bool store_meta(cyng::db::session sp
				, boost::uuids::uuid pk
				, std::string const& trx
				, std::size_t idx
				, cyng::object ro_ime
				, cyng::object act_ime
				, cyng::object val_time
				, cyng::buffer_t const& client_id	//	gateway - formatted
				, cyng::buffer_t const& server_id	//	server - formatted
				, cyng::object status
				, obis profile);

			bool store_data(cyng::db::session sp
				, boost::uuids::uuid pk
				, std::string const& trx
				, std::size_t idx
				, obis const& code
				, std::uint8_t unit
				, std::string unit_name
				, std::string type_name	//	CYNG data type name
				, std::int8_t scaler	//	scaler
				, cyng::object raw		//	raw value
				, cyng::object value);

		private:
			const cyng::table::meta_map_t& mt_;
			const std::string schema_;
			const std::uint32_t source_;
			const std::uint32_t channel_;
			const std::string target_;

			boost::uuids::random_generator rgn_;
			readout ro_;
		};

	}	//	sml
}

#endif
