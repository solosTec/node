/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SML_EXPORTER_INFLUXDB_H
#define NODE_SML_EXPORTER_INFLUXDB_H


#include <smf/sml/defs.h>
#include <smf/sml/units.h>
#include <smf/sml/intrinsics/obis.h>

#include <cyng/intrinsics/sets.h>
#include <cyng/compatibility/io_service.h>
#include <cyng/object.h>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
//#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
//#include <boost/asio/ssl/error.hpp>
//#include <boost/asio/ssl/stream.hpp>

namespace node	
{
	namespace sml	
	{
		/**
		 * walk down SML message body recursively, collect data
		 * and send data to influxdb
		 */
		struct readout;
		class influxdb_exporter
		{
		public:
			influxdb_exporter(cyng::io_service_t&
				, std::string const& host
				, std::string const& service
				, std::string const& protocol
				, std::string const& cert
				, std::string const& db
				, std::string const& series
				, std::uint32_t source
				, std::uint32_t channel
				, std::string const& target);
			

			/**
			 * read incoming input
			 */
			void read(cyng::tuple_t const&);

			static int create_db(cyng::io_service_t& 
				, std::string const& host
				, std::string const& service
				, std::string const& protocol
				, std::string const& cert
				, std::string const& db);

			static int show_db(cyng::io_service_t&
				, std::string const& host
				, std::string const& service
				, std::string const& protocol
				, std::string const& cert);

			static int drop_db(cyng::io_service_t&
				, std::string const& host
				, std::string const& service
				, std::string const& protocol
				, std::string const& cert
				, std::string const& db);
		private:
			/**
			 * read SML message.
			 */
			void read_msg(readout&, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);
			void read_body(readout& ro, message_e code, cyng::tuple_t const& tpl);
			void read_get_proc_parameter_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);
			void read_get_profile_list_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);

			void push_over_http(std::uint32_t profile
				, cyng::buffer_t client_id
				, cyng::buffer_t server_id
				, cyng::object actTime
				, cyng::object valTime
				, cyng::object status
				, cyng::param_map_t const& pmap);

		private:
			cyng::io_service_t& ioc_;

			std::string const host_;
			std::string const service_;
			std::string const protocol_;
			std::string const cert_;
			std::string const db_;
			std::string const series_;

			std::uint32_t const source_;
			std::uint32_t const channel_;
			std::string const target_;
		};

		/**
		 * Convert a timestamp into a string that can used in the influxdb line protocol
		 */
		std::string to_line_protocol(std::chrono::system_clock::time_point tp);
		std::string to_line_protocol(cyng::object);
		std::string to_line_protocol(std::string s);

	}	//	sml
}	

#endif