/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#ifndef NODE_SML_EXPORTER_DB_H
#define NODE_SML_EXPORTER_DB_H


#include <smf/sml/defs.h>
#include <smf/sml/intrinsics/obis.h>
#include <smf/sml/units.h>
#include <smf/sml/intrinsics/obis.h>
#include <cyng/vm/context.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/object.h>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	namespace sml
	{
		/**
		 * walk down SML message body recursively, collect data
		 * and create an XML document.
		 */
		class sml_reader
		{
			struct readout
			{
				readout(boost::uuids::uuid);

				void reset(boost::uuids::uuid, std::size_t);

				readout& set_trx(cyng::buffer_t const&);
				readout& set_index(std::size_t);
				readout& set_value(std::string const&, cyng::object);
				cyng::object get_value(std::string const&) const;

				boost::uuids::uuid pk_;
				std::size_t idx_;	//!< message index
				std::string trx_;
				cyng::param_map_t values_;
			};

		public:
			sml_reader();

			/**
			 * reset exporter
			 */
			void reset();

			cyng::vector_t read(cyng::tuple_t const&, std::size_t idx);

		private:
			/**
			 * read SML message.
			 */
			cyng::vector_t read_msg(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator, std::size_t idx);
			cyng::vector_t read_body(cyng::object, cyng::object);
			cyng::vector_t read_public_open_request(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_public_open_response(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_public_close_request(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_get_profile_list_response(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_get_proc_parameter_response(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_get_proc_parameter_request(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_attention_response(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			void read_time(std::string const&, cyng::object);
			std::vector<obis> read_param_tree_path(cyng::object);
			obis read_obis(cyng::object);
			std::uint8_t read_unit(std::string const&, cyng::object);
			std::int8_t read_scaler(cyng::object);
			std::string read_string(std::string const&, cyng::object);
			void read_value(obis, std::int8_t, std::uint8_t, cyng::object);
			void read_parameter(obis, cyng::object);
			std::string read_server_id(cyng::object);
			std::string read_client_id(cyng::object);

			void read_period_list(std::vector<obis> const&, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_period_entry(std::vector<obis> const&, std::size_t, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_param_tree(std::size_t, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

		private:
			boost::uuids::random_generator rgn_;
			readout ro_;
		};

	}	//	sml
}

#endif