/*
* Copyright Sylko Olzscher 2016
*
* Use, modification, and distribution is subject to the Boost Software
* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
* http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef NODDY_SML_ABL_EXPORTER_H
#define NODDY_SML_ABL_EXPORTER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <noddy/sml/sml_defs.h>
#include <noddy/m2m/intrinsics/obis.h>
#include <noddy/m2m/intrinsics/ctrl_address.h>

#include <cyy/intrinsics/sets.hpp>
#include <boost/filesystem.hpp>

namespace noddy	
{
	namespace sml	
	{
		class sml_abl_exporter
		{
		public:
			sml_abl_exporter(std::string const& dir, std::string const& prefix);

			void extract(cyy::object const&, std::size_t);
			void extract(cyy::tuple_t const&, std::size_t);
			//bool save(boost::filesystem::path const&);

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

		protected:
			void traverse(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end
				, std::size_t);

			void traverse_body(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_open_response(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_open_request(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_close_response(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_get_profile_list_response(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_get_process_parameter_response(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_get_profile_pack_request(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_get_profile_list_request(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_get_process_parameter_request(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_set_process_parameter_request(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_attention_response(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_list_of_period_entries(std::ofstream&
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end
				, m2m::obis const& object_code);

			void traverse_period_entry(std::ofstream&
				, std::uint32_t counter
				, m2m::obis const& object_code
				, cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			cyy::object traverse_time(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			cyy::object traverse_time(cyy::object);

			void traverse_sml_tree(cyy::object obj
				, std::size_t pos = 0
				, std::size_t depth = 0);

			void traverse_sml_tree(cyy::tuple_t::const_iterator
				, cyy::tuple_t::const_iterator
				, std::size_t pos = 0
				, std::size_t depth = 0);

			void traverse_child_list(cyy::tuple_t::const_iterator
				, cyy::tuple_t::const_iterator
				, std::size_t depth = 0);

			void traverse_sml_proc_par_value(cyy::object);

			void traverse_sml_proc_par_value(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_tuple_entry(cyy::tuple_t::const_iterator begin
				, cyy::tuple_t::const_iterator end);

			void traverse_tuple_entry(cyy::object);


			m2m::obis traverse_parameter_tree_path(cyy::object);

			void emit_object(m2m::obis const&);

			/**
			 * create a unique output file name
			 */
			boost::filesystem::path unique_file_name(std::chrono::system_clock::time_point const&
				, noddy::m2m::ctrl_address const&) const;


		private:
			const boost::filesystem::path out_dir_;
			const std::string prefix_;

			std::string trx_;	//!< last transaction (open response)
			cyy::buffer_t	client_id_;		//!<	last client ID (open response)
			cyy::buffer_t	server_id_;		//!<	last server ID (open response)
			std::uint32_t	choice_;		//!<	last message type
		};

	}	//	sml
}	

#endif	//	NODDY_SML_ABL_EXPORTER_H