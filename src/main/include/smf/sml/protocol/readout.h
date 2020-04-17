/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_READOUT_H
#define NODE_LIB_READOUT_H


#include <smf/sml/defs.h>
#include <smf/sml/intrinsics/obis.h>

#include <cyng/object.h>
#include <cyng/intrinsics/sets.h>

#include <boost/uuid/uuid.hpp>

namespace node
{
	namespace sml
	{
		struct readout
		{
			readout();

			/**
			 * Clear all values.
			 */
			void reset();

			readout& set_trx(cyng::buffer_t const&);
			readout& set_value(std::string const&, cyng::object);
			readout& set_pk(boost::uuids::uuid);

			/**
			 * overwrite existing values
			 */
			readout& set_value(cyng::param_t const&);
			readout& set_value(cyng::param_map_t&&);
			readout& set_value(obis, cyng::object);
			readout& set_value(obis, std::string, cyng::object);
			cyng::object get_value(std::string const&) const;
			cyng::object get_value(obis) const;
			std::string get_string(obis) const;

			std::string read_server_id(cyng::object obj);
			std::string read_client_id(cyng::object obj);

			/** 
			 * Read the object as choice and returns one the 
			 * SML message types (like BODY_OPEN_REQUEST, BODY_OPEN_RESPONSE, ...)
			 * and the following body as tuple.
			 */
			std::pair<sml_message, cyng::tuple_t> read_choice(cyng::object obj);
			std::pair<sml_message, cyng::tuple_t> read_msg(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);

			bool read_public_open_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);
			bool read_public_open_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);
			bool read_public_close_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);
			bool read_public_close_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);

			std::pair<obis_path_t, cyng::param_t> read_get_proc_parameter_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);
			std::pair<obis_path_t, cyng::object> read_get_proc_parameter_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);

			std::pair<obis_path_t, cyng::param_map_t> read_get_profile_list_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);
			obis_path_t read_get_profile_list_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);

			std::pair<obis_path_t, cyng::param_t> read_set_proc_parameter_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);


			std::string trx_;
			cyng::buffer_t	server_id_;
			cyng::buffer_t	client_id_;
			cyng::param_map_t values_;
		};

		//
		//	helper
		//

		/**
		 * Take the elements of the buffer as ASCII code and build a string
		 *
		 * Precondition is that the object contains a binary buffer (cyng::buffer_t)
		 */
		cyng::object read_string(cyng::object);

		std::int8_t read_scaler(cyng::object);
		std::uint8_t read_unit(cyng::object);

		/**
		 * Read an object as buffer and initialize
		 * an OBIS code with the buffer.
		 * If buffer has a length other then 6 and empty OBIS
		 * code will be produced.
		 */
		obis read_obis(cyng::object);

		/**
		 * Expects an tuple of buffers that will be converted 
		 * into an OBIS path
		 */
		obis_path_t read_param_tree_path(cyng::object);

		cyng::param_t read_param_tree(std::size_t, cyng::object);
		cyng::param_t read_param_tree(std::size_t, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

		/**
		 * A SML parameter consists of a type (PROC_PAR_VALUE, PROC_PAR_TIME, ...) and a value.
		 * This functions read the parameter and creates an attribute that contains these
		 * two informations
		 *
		 * Precondition is that the object contains a list/tuple with the a size of 2.
		 */
		cyng::attr_t read_parameter(cyng::object);


		/**
		 * SML defines 2 representations of time: TIME_TIMESTAMP and TIME_SECINDEX.
		 * This functions decodes the type and produces an object that contains the
		 * value. If the type is TIME_TIMESTAMP the return value is a std::chrono::system_clock::time_point.
		 * Otherwise the value is u32 value.
		 *
		 * Precondition is that the object contains a list/tuple with the a size of 2.
		 */
		cyng::object read_time(cyng::object);

		/**
		 * Convert data types of some specific OBIS values
		 */
		cyng::param_t customize_value(obis code, cyng::object);

		/**
		 * build a vector of devices
		 */
		cyng::param_t collect_devices(obis code, cyng::tuple_t const& tpl);

		/**
		 * build a vector of devices
		 */
		cyng::param_t collect_iec_devices(cyng::tuple_t const& tpl);

		/**
		 * read a listor period entries
		 */
		cyng::param_map_t read_period_list(cyng::tuple_t::const_iterator
			, cyng::tuple_t::const_iterator);

		/**
		 * read a SML_PeriodEntry record
		 */
		cyng::param_t read_period_entry(std::size_t
			, cyng::tuple_t::const_iterator
			, cyng::tuple_t::const_iterator);

		/**
		 * Takes the type information (code, scaler, unit) and transform the obj parameter
		 * in a meaningfull string. All calculations for positionioning the decimal point
		 * will done.
		 */
		cyng::object read_value(obis code, std::int8_t scaler, std::uint8_t unit, cyng::object obj);

	}	//	sml
}

#endif