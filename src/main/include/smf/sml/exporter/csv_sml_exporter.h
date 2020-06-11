/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_EXPORTER_CSV_H
#define NODE_SML_EXPORTER_CSV_H


#include <smf/sml/defs.h>
#include <smf/sml/units.h>
#include <smf/sml/protocol/readout.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/object.h>
#include <fstream>
#include <cyng/compatibility/file_system.hpp>

namespace node
{
	namespace sml
	{
		/**
		 * To build a usefull filename constructor with source,
		 * channel and target info is required,
		 */
		cyng::filesystem::path get_csv_filename(std::string prefix
			, std::string suffix
			, std::uint32_t source
			, std::uint32_t channel
			, std::string const& target);

		/**
		 * walk down SML message body recursively, collect data
		 * and write data into SQL database.
		 */
		class csv_exporter
		{
		public:
			csv_exporter(cyng::filesystem::path root_dir
				, std::string prefix
				, std::string suffix
				, bool header);
			csv_exporter(cyng::filesystem::path root_dir
				, std::string prefix
				, std::string suffix
				, bool header
				, std::uint32_t source
				, std::uint32_t channel
				, std::string const& target);
			virtual ~csv_exporter();


			/**
			 * reset exporter
			 */
			void reset();

			void write(cyng::tuple_t const&);

		private:
			/**
			 * read SML message.
			 */
			void read_msg(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_body(message_e code, cyng::tuple_t tpl);
			void read_get_profile_list_response(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_get_proc_parameter_response(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_attention_response(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			void read_time(std::string const&, cyng::object);
			std::vector<obis> read_param_tree_path(cyng::object);
			obis read_obis(cyng::object);
			std::uint8_t read_unit(std::string const&, cyng::object);
			std::int8_t read_scaler(cyng::object);
			std::string read_string(std::string const&, cyng::object);

			/**
			 * @return CYNG data type tag
			 */
			std::size_t read_value(obis, std::int8_t, std::uint8_t, cyng::object);
			void read_parameter(obis, cyng::object);
			std::string read_server_id(cyng::object);
			std::string read_client_id(cyng::object);

			void read_period_list(std::vector<obis> const&, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_period_entry(std::vector<obis> const&, std::size_t, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_param_tree(std::size_t, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			/**
			 * Write optional CSV header
			 */
			void write_header();

		private:
			cyng::filesystem::path const root_dir_;
			std::string const prefix_;
			std::string const suffix_;
			bool header_;

			std::uint32_t const source_;
			std::uint32_t const channel_;
			std::string const target_;

			readout ro_;
			std::ofstream	ofstream_;
		};

	}	//	sml
}

#endif
