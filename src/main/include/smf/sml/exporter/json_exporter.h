/*
* Copyright Sylko Olzscher 2017
*
* Use, modification, and distribution is subject to the Boost Software
* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
* http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef NODDY_SML_JSON_EXPORTER_H
#define NODDY_SML_JSON_EXPORTER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <noddy/sml/sml_defs.h>
#include <noddy/m2m/intrinsics/obis.h>
#include <cyy/intrinsics/sets.hpp>
#include <cyy/scheduler/logging/log.h>
#include <boost/filesystem.hpp>

namespace noddy	
{
	namespace sml	
	{
		class sml_json_exporter
		{
		public:
			sml_json_exporter(logger_type&);

			void extract(cyy::object const&, std::size_t);
			void extract(cyy::tuple_t const&, std::size_t);
			bool save(boost::filesystem::path const&);
			bool print(std::ostream&);

			/**
			 * destroys the tree and replaces it with either an empty one 
			 */
			void reset();


			/**
			 * @return a textual representation of the last read transaction
			 */
			std::string const& get_last_trx() const;

			cyy::buffer_t const& get_last_client_id() const;
			cyy::buffer_t const& get_last_server_id_() const;

			/**
			 * @return last processed message type
			 */
			std::uint32_t get_last_choice() const;

			/**
			 * @return root element
			 */
			cyy::vector_t const& get_root() const;


		protected:
			void traverse(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end
				, std::size_t);

			void traverse_body(cyy::tuple_t&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_open_response(cyy::tuple_t&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_open_request(cyy::tuple_t&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_close_response(cyy::tuple_t&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_get_profile_list_response(cyy::tuple_t&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_get_process_parameter_response(cyy::tuple_t&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_get_profile_pack_request(cyy::tuple_t&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_get_profile_list_request(cyy::tuple_t&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_get_process_parameter_request(cyy::tuple_t&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_set_process_parameter_request(cyy::tuple_t&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_attention_response(cyy::tuple_t&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			cyy::vector_t traverse_list_of_period_entries(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end
				, m2m::obis const& object_code);

			cyy::tuple_t traverse_period_entry(std::uint32_t counter
				, m2m::obis const& object_code
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_time(cyy::tuple_t&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_time(cyy::tuple_t&
				, cyy::object
				, std::string const& name);

			void traverse_sml_tree(cyy::tuple_t&
				, cyy::object obj
				, std::size_t pos = 0
				, std::size_t depth = 0);

			void traverse_sml_tree(cyy::tuple_t&
				, cyy::tuple_t::const_iterator
				, cyy::tuple_t::const_iterator
				, std::size_t pos = 0
				, std::size_t depth = 0);

			void traverse_child_list(cyy::tuple_t&
				, cyy::tuple_t::const_iterator
				, cyy::tuple_t::const_iterator
				, std::size_t depth = 0);

			void traverse_sml_proc_par_value(cyy::tuple_t&
				, cyy::object);

			void traverse_sml_proc_par_value(cyy::tuple_t&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_tuple_entry(cyy::tuple_t&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_tuple_entry(cyy::tuple_t&
				, cyy::object);


			m2m::obis traverse_parameter_tree_path(cyy::tuple_t&
				, cyy::object);

			void emit_object(cyy::tuple_t&
				, m2m::obis const&);

			void emit_ctrl_address(cyy::tuple_t& node
				, cyy::buffer_t const& buffer);

			void emit_value(cyy::tuple_t& node
				, std::string const& name
				, cyy::object const&);

		private:
			logger_type& logger_;

			/**
			 * The JSON document
			 */
			cyy::vector_t root_;

			std::string trx_;	//!< last transaction
			cyy::buffer_t	client_id_;		//!<	last client ID
			cyy::buffer_t	server_id_;		//!<	last server ID
			std::uint32_t	choice_;		//!<	last message type
		};

	}	//	sml
}	

#endif	//	NODDY_SML_JSON_EXPORTER_H
