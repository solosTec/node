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
#include <cyng/vm/controller.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/object.h>

namespace node
{
	namespace sml
	{
		/**
		* walk down SML message body recursively, collect data
		* and create an XML document.
		*/
		class db_exporter
		{
			struct context
			{
				context append_child(std::string const&);
			};

		public:
			db_exporter(cyng::controller&);
			db_exporter(cyng::controller&
				, std::uint32_t source
				, std::uint32_t channel
				, std::string const& target);


			/**
			* destroys the tree and replaces it with either an empty one
			*/
			void reset();

			void read(cyng::tuple_t const&, std::size_t idx);

			/**
			* To build a usefull filename constructor with source,
			* channel and target info is required,
			*/
			//boost::filesystem::path get_filename() const;

		private:
			/**
			* read SML message.
			*/
			void read_msg(cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator, std::size_t idx);
			void read_body(context, cyng::object, cyng::object);
			void read_public_open_request(context, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_public_open_response(context, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_get_profile_list_response(context, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_get_proc_parameter_response(context, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_attention_response(context, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

			void read_time(context, cyng::object);
			std::vector<obis> read_param_tree_path(context, cyng::object);
			obis read_obis(context, cyng::object);
			std::uint8_t read_unit(context, cyng::object);
			std::int8_t read_scaler(context, cyng::object);
			void read_value(context, obis, std::int8_t, std::uint8_t, cyng::object);
			void read_parameter(context, obis, cyng::object);

			void read_period_list(context, std::vector<obis> const&, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_period_entry(context, std::vector<obis> const&, std::size_t, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);
			void read_param_tree(context, std::size_t, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

		private:
			/**
			 * The processor
			 */
			cyng::controller& vm_;

			const std::uint32_t source_;
			const std::uint32_t channel_;
			const std::string target_;

		};

	}	//	sml
}

#endif