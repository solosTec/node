/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_READER_H
#define NODE_LIB_READER_H


#include <smf/sml/defs.h>
#include <smf/sml/intrinsics/obis.h>
#include <smf/sml/units.h>
#include <smf/sml/protocol/readout.h>
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
		class reader
		{

		public:
			reader();

			/**
			 * reset exporter
			 */
			void reset();

			cyng::vector_t read(cyng::tuple_t const&, std::size_t idx);

			cyng::vector_t read_choice(cyng::tuple_t const&);
			void set_trx(cyng::buffer_t const&);

		private:
			/**
			 * read SML message.
			 */
			cyng::vector_t read_msg(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator, std::size_t idx);
			cyng::vector_t read_body(cyng::object, cyng::object);
			cyng::vector_t read_public_open_request(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_public_open_response(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_public_close_request(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_public_close_response(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_get_profile_list_request(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_get_profile_list_response(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_get_proc_parameter_response(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_get_proc_parameter_request(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_set_proc_parameter_request(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_get_list_request(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_get_list_response(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			cyng::vector_t read_attention_response(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			cyng::vector_t read_set_proc_parameter_request_tree(std::vector<obis> path
				, std::size_t depth
				, cyng::tuple_t::const_iterator pos
				, cyng::tuple_t::const_iterator end);

			cyng::vector_t read_get_proc_parameter_response(std::vector<obis> path
				, std::size_t depth
				, cyng::tuple_t::const_iterator pos
				, cyng::tuple_t::const_iterator end);
			void read_get_proc_single_parameter(cyng::tuple_t::const_iterator pos
				, cyng::tuple_t::const_iterator end);
			void read_get_proc_single_parameter(cyng::object);
			void read_get_proc_multiple_parameters(cyng::object);
			//void read_get_proc_memory(cyng::object);

			cyng::vector_t read_tree_list(std::vector<obis> path, cyng::object, std::size_t depth);

			cyng::object read_time(std::string const&, cyng::object);
			std::vector<obis> read_param_tree_path(cyng::object);
			obis read_obis(cyng::object);
			std::uint8_t read_unit(std::string const&, cyng::object);
			std::int8_t read_scaler(cyng::object);
			std::string read_string(std::string const&, cyng::object);
			/**
			 * @param use_vector if false the value is noted as a single object otherwise
			 * a vector of obis code / value pairs is notes. This is usefufull to distinguish between several values in a list.
			 */
			cyng::object read_value(obis, std::int8_t, std::uint8_t, cyng::object);
			cyng::attr_t read_parameter(cyng::object);
			std::string read_server_id(cyng::object);
			std::string read_client_id(cyng::object);

			void read_sml_list(obis, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_list_entry(obis, std::size_t index, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);

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