/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_EXPORTER_XML_SML_H
#define NODE_SML_EXPORTER_XML_SML_H


#include <smf/sml/defs.h>
#include <smf/sml/intrinsics/obis.h>
#include <smf/sml/units.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/object.h>

#include <pugixml.hpp>
#include <cyng/compatibility/file_system.hpp>

namespace node	
{
	namespace sml	
	{
		/**
		 * walk down SML message body recursively, collect data
		 * and create an XML document.
		 */
		class xml_exporter
		{
		public:
			xml_exporter();
			xml_exporter(std::string const&, std::string const&);
			xml_exporter(std::string const&
				, std::string const&
				, std::uint32_t source
				, std::uint32_t channel
				, std::string const& target);
			

			/**
			 * destroys the tree and replaces it with either an empty one 
			 */
			void reset();

			/**
			 * read incoming input
			 */
			void read(cyng::tuple_t const&);

			/**
			 * Write XML file
			 */
			bool write(cyng::filesystem::path const&);

			/**
			 * To build a usefull filename constructor with source,
			 * channel and target info is required,
			 */
			cyng::filesystem::path get_filename() const;

		private:
			/**
			 * read SML message.
			 */
			void read_msg(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_body(pugi::xml_node, cyng::object, cyng::object);
			void read_public_open_request(pugi::xml_node, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_public_open_response(pugi::xml_node, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_get_profile_list_response(pugi::xml_node, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_get_proc_parameter_response(pugi::xml_node, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_attention_response(pugi::xml_node, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			void read_time(pugi::xml_node, cyng::object);
			std::vector<obis> read_param_tree_path(pugi::xml_node, cyng::object);
			obis read_obis(pugi::xml_node, cyng::object);
			std::uint8_t read_unit(pugi::xml_node, cyng::object);
			std::int8_t read_scaler(pugi::xml_node, cyng::object);
			std::string read_string(pugi::xml_node, cyng::object);
			void read_value(pugi::xml_node, obis, std::int8_t, std::uint8_t, cyng::object);
			void read_parameter(pugi::xml_node, obis, cyng::object);
			std::string read_server_id(pugi::xml_node, cyng::object);
			std::string read_client_id(pugi::xml_node, cyng::object);

			void read_period_list(pugi::xml_node, std::vector<obis> const&, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_period_entry(pugi::xml_node, std::vector<obis> const&, std::size_t, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_param_tree(pugi::xml_node, std::size_t, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

		private:
			/**
			 * The XML document
			 */
			pugi::xml_document doc_;
			pugi::xml_node root_;

			std::string const encoding_;
			std::string const root_name_;

			std::uint32_t const source_;
			std::uint32_t const channel_;
			std::string const target_;

		};

	}	//	sml
}	

#endif