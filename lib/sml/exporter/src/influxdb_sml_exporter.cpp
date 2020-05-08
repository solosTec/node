/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/exporter/influxdb_sml_exporter.h>
#include <NODE_project_info.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/scaler.h>
#include <smf/sml/protocol/readout.h>
#include <smf/mbus/defs.h>
#include <smf/mbus/units.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/io/serializer.h>
#include <cyng/value_cast.hpp>
#include <cyng/factory.h>
#include <cyng/chrono.h>

#include <boost/core/ignore_unused.hpp>

namespace node
{
	namespace sml	
	{
		influxdb_exporter::influxdb_exporter(cyng::io_service_t& ioc
			, std::string const& host
			, std::string const& service
			, std::string const& protocol
			, std::string const& cert
			, std::string const& db
			, std::string const& series
			, std::uint32_t source
			, std::uint32_t channel
			, std::string const& target)
		: ioc_(ioc)
			, host_(host)
			, service_(service)
			, protocol_(protocol)
			, cert_(cert)
			, db_(db)
			, series_(series)
			, source_(source)
			, channel_(channel)
			, target_(target)
		{
			//if (tls_) {
			//	//
			//	//	load certificates
			//	//
			//}

		}

		void influxdb_exporter::read(cyng::tuple_t const& tpl)
		{
			readout ro;
			read_msg(ro, tpl.begin(), tpl.end());
		}

		void influxdb_exporter::read_msg(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			//std::pair<std::uint16_t, cyng::tuple_t> 
			auto const r = ro.read_msg(pos, end);

			//
			//	Read the SML message body
			//
			return read_body(ro, r.first, r.second);

		}

		void influxdb_exporter::read_body(readout& ro, message_e code, cyng::tuple_t const& tpl)
		{
			switch (code)
			{
			case message_e::OPEN_REQUEST:
				break;
			case message_e::OPEN_RESPONSE:
				break;
			case message_e::CLOSE_REQUEST:
				break;
			case message_e::CLOSE_RESPONSE:
				break;
			case message_e::GET_PROFILE_PACK_REQUEST:
				break;
			case message_e::GET_PROFILE_PACK_RESPONSE:
				break;
			case message_e::GET_PROFILE_LIST_REQUEST:
				break;
			case message_e::GET_PROFILE_LIST_RESPONSE:
				read_get_profile_list_response(ro, tpl.begin(), tpl.end());
				break;
			case message_e::GET_PROC_PARAMETER_REQUEST:
				break;
			case message_e::GET_PROC_PARAMETER_RESPONSE:
				read_get_proc_parameter_response(ro, tpl.begin(), tpl.end());
				break;
			case message_e::SET_PROC_PARAMETER_REQUEST:
				break;
			case message_e::SET_PROC_PARAMETER_RESPONSE:
				break;
			case message_e::GET_LIST_REQUEST:
				break;
			case message_e::GET_LIST_RESPONSE:
				break;
			case message_e::GET_COSEM_REQUEST:
				break;
			case message_e::GET_COSEM_RESPONSE:
				break;
			case message_e::SET_COSEM_REQUEST:
				break;
			case message_e::SET_COSEM_RESPONSE:
				break;
			case message_e::ACTION_COSEM_REQUEST:
				break;
			case message_e::ACTION_COSEM_RESPONSE:
				break;

			case message_e::ATTENTION_RESPONSE:
				break;
			default:
				break;
			}
		}

		void influxdb_exporter::read_get_proc_parameter_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			//	std::pair<obis_path_t, cyng::param_t>
			auto const r = ro.read_get_proc_parameter_response(pos, end);

#ifdef __DEBUG
			std::cerr
				<< "\n\n"
				<< std::string(72, '.')
				<< "\n\n"
				<< target_
				<< " => read_get_proc_parameter_response "
				<< to_hex(r.first, ':')
				<< " - "
				<< r.second.first
				<< ": "
				<< cyng::io::to_str(r.second.second)
				<< "\n\n"
				<< std::string(72, '.')
				<< std::endl
				;
#endif

		}

		void influxdb_exporter::read_get_profile_list_response(readout& ro, cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			auto const r = ro.read_get_profile_list_response(pos, end);

#ifdef _DEBUG
			std::cerr
				<< "\n\n"
				<< std::string(72, '.')
				<< "\n\n"
				<< target_
				<< " ==> read_get_profile_list_response "
				<< to_hex(r.first, ':')
				<< " - "
				<< cyng::io::to_str(r.second)
				<< "\n\n"
				<< std::string(72, '.')
				<< std::endl
				;
#endif
			BOOST_ASSERT(!r.first.empty());
			//r.first.front();

			//
			//	profile
			//
			auto const q = r.first.front().get_quantities();

			
			//ro.trx_;
			//	8181C78611FF = 15 min profile
			//	power@solostec ==> read_get_profile_list_response 8181C78611FF - %(("010000090B00":2020-03-24 11:29:48.00000000),("0100010800FF":1452.1),("0100020800FF":56113.9),("81062B070000":-58),("8181C78203FF":EMH))
			
			push_over_http(q
				, ro.client_id_
				, ro.server_id_
				, ro.get_value("actTime")
				, ro.get_value("valTime")
				, ro.get_value("status")
				, r.second);

		}

		void influxdb_exporter::push_over_http(std::uint32_t profile
			, cyng::buffer_t client_id
			, cyng::buffer_t server_id
			, cyng::object actTime
			, cyng::object valTime
			, cyng::object status
			, cyng::param_map_t const& pmap)
		{
			//
			//	create stream
			//
			boost::beast::tcp_stream stream(ioc_);

			//
			// Look up the domain name
			//
			boost::asio::ip::tcp::resolver resolver(ioc_);
			auto const results = resolver.resolve(host_, service_);

			//
			// Make the connection on the IP address we get from a lookup
			//
			boost::beast::get_lowest_layer(stream).connect(results);

			//
			// Set up an HTTP POST request message HTTP 1.1
			//
			boost::beast::http::request<boost::beast::http::string_body> req{ boost::beast::http::verb::post, "/write?db="+ db_, 11 };
			req.set(boost::beast::http::field::host, host_);
			req.set(boost::beast::http::field::user_agent, NODE::version_string);

			//
			//	generate data points
			//	example
			//  SML,profile=17,target=power@solostec,server=01-a815-74314504-01-02 status=00020240,010000090B00=1585129472000000000 1585130105272160500
			//  SML,profile=17,target=power@solostec,server=01-a815-74314504-01-02 status=00020240,0100010800FF="1452.1" 1585130105272259000
			//  SML,profile=17,target=power@solostec,server=01-a815-74314504-01-02 status=00020240,0100020800FF="56113.9" 1585130105272317600
			//  SML,profile=17,target=power@solostec,server=01-a815-74314504-01-02 status=00020240,81062B070000=-59i 1585130105272368700
			//  SML,profile=17,target=power@solostec,server=01-a815-74314504-01-02 status=00020240,8181C78203FF="EMH" 1585130105272424500			//
			//	

			std::stringstream ss;
			ss
				<< series_
				//
				//	tags:
				//
				<< ",profile="
				<< profile
				<< ",target="
				<< target_
				<< ",server="
				<< from_server_id(server_id)

				//
				//	fields:
				//
				<< ' '
				<< "status="
				<< to_line_protocol(status)
// 				<< ",valTime="
// 				<< to_line_protocol(valTime)
				;
			for (auto const& v : pmap) {
				ss
					<< ","
					<< v.first
					<< "="
					<< to_line_protocol(v.second)
					;
			}

			//
			//	timestamp
			//
			ss
				<< ' '
				<< to_line_protocol(actTime)
				<< '\n'
				;
#ifdef _DEBUG
			std::cerr
				<< "\n\n"
				<< std::string(72, '.')
				<< "\n\n"
				<< ss.str()
				<< std::endl
				<< "actTime: "
				<< cyng::io::to_str(actTime)
				<< std::endl
				<< "valTime: "
				<< cyng::io::to_str(valTime)
				<< std::endl
				;
#endif

			req.body() = ss.str();
			req.prepare_payload();


			//
			// Send the HTTP request to the remote host
			//
			boost::beast::http::write(stream, req);

			// This buffer is used for reading and must be persisted
			boost::beast::flat_buffer buffer;

			// Declare a container to hold the response
			boost::beast::http::response<boost::beast::http::dynamic_body> res;

			// Receive the HTTP response
			boost::beast::http::read(stream, buffer, res);

			// Write the message to standard out
			std::cout << res << std::endl;

		}


		int influxdb_exporter::create_db(cyng::io_service_t& ioc
			, std::string const& host
			, std::string const& service
			, std::string const& protocol
			, std::string const& cert
			, std::string const& db)
		{
			std::cout
				<< "***info : "
				<< host
				<< ':'
				<< service
				<< "/query --data-urlencode \"q=CREATE DATABASE "
				<< db 
				<< "\""
				<< std::endl
				;

			//
			//	create stream
			//
			boost::beast::tcp_stream stream(ioc);

			//
			// Look up the domain name
			//
			boost::asio::ip::tcp::resolver resolver(ioc);
			auto const results = resolver.resolve(host, service);

			//
			// Make the connection on the IP address we get from a lookup
			//
			boost::beast::get_lowest_layer(stream).connect(results);

			//
			// Set up an HTTP POST request message HTTP 1.1
			//
			boost::beast::http::request<boost::beast::http::string_body> req{ boost::beast::http::verb::post, "/query?q=CREATE+DATABASE+"+db, 11 };
			req.set(boost::beast::http::field::host, host);
			req.set(boost::beast::http::field::user_agent, NODE::version_string);
			//req.body() = ("q=CREATE+DATABASE+" + db);
			req.prepare_payload();


			//
			// Send the HTTP request to the remote host
			//
			boost::beast::http::write(stream, req);

			// This buffer is used for reading and must be persisted
			boost::beast::flat_buffer buffer;

			// Declare a container to hold the response
			boost::beast::http::response<boost::beast::http::dynamic_body> res;

			// Receive the HTTP response
			boost::beast::http::read(stream, buffer, res);

			// Write the message to standard out
			std::cout << res << std::endl;


			return EXIT_SUCCESS;
		}

		int influxdb_exporter::show_db(cyng::io_service_t& ioc
			, std::string const& host
			, std::string const& service
			, std::string const& protocol
			, std::string const& cert)
		{
			std::cout
				<< "***info : "
				<< host
				<< ':'
				<< service
				<< "/query --data-urlencode \"q=SHOW DATABASES\""
				<< std::endl
				;

			//
			//	create stream
			//
			boost::beast::tcp_stream stream(ioc);

			//
			// Look up the domain name
			//
			boost::asio::ip::tcp::resolver resolver(ioc);
			auto const results = resolver.resolve(host, service);

			//
			// Make the connection on the IP address we get from a lookup
			//
			boost::beast::get_lowest_layer(stream).connect(results);

			//
			// Set up an HTTP POST request message HTTP 1.1
			//
			boost::beast::http::request<boost::beast::http::string_body> req{ boost::beast::http::verb::get, "/query?q=SHOW+DATABASES", 11 };
			req.set(boost::beast::http::field::host, host);
			req.set(boost::beast::http::field::user_agent, NODE::version_string);
			//req.body() = "--data-urlencode \"q=SHOW DATABASES\"";
			req.prepare_payload();


			//
			// Send the HTTP request to the remote host
			//
			boost::beast::http::write(stream, req);

			// This buffer is used for reading and must be persisted
			boost::beast::flat_buffer buffer;

			// Declare a container to hold the response
			boost::beast::http::response<boost::beast::http::dynamic_body> res;

			// Receive the HTTP response
			boost::beast::http::read(stream, buffer, res);

			// Write the message to standard out
			std::cout << res << std::endl;

			return EXIT_SUCCESS;
		}

		int influxdb_exporter::drop_db(cyng::io_service_t& ioc
			, std::string const& host
			, std::string const& service
			, std::string const& protocol
			, std::string const& cert
			, std::string const& db)
		{
			std::cout
				<< "***info : "
				<< host
				<< ':'
				<< service
				<< "/query --data-urlencode \"q=DROP DATABASE\" "
				<< db
				<< std::endl
				;

			//
			//	create stream
			//
			boost::beast::tcp_stream stream(ioc);

			//
			// Look up the domain name
			//
			boost::asio::ip::tcp::resolver resolver(ioc);
			auto const results = resolver.resolve(host, service);

			//
			// Make the connection on the IP address we get from a lookup
			//
			boost::beast::get_lowest_layer(stream).connect(results);

			//
			// Set up an HTTP POST request message HTTP 1.1
			//
			boost::beast::http::request<boost::beast::http::string_body> req{ boost::beast::http::verb::post, "/query?q=DROP+DATABASE+" + db, 11 };
			req.set(boost::beast::http::field::host, host);
			req.set(boost::beast::http::field::user_agent, NODE::version_string);
			req.prepare_payload();


			//
			// Send the HTTP request to the remote host
			//
			boost::beast::http::write(stream, req);

			// This buffer is used for reading and must be persisted
			boost::beast::flat_buffer buffer;

			// Declare a container to hold the response
			boost::beast::http::response<boost::beast::http::dynamic_body> res;

			// Receive the HTTP response
			boost::beast::http::read(stream, buffer, res);

			// Write the message to standard out
			std::cout << res << std::endl;

			return EXIT_SUCCESS;
		}

		std::string to_line_protocol(std::chrono::system_clock::time_point tp)
		{
			return std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count());
		}
		std::string to_line_protocol(cyng::object obj)
		{
			switch (obj.get_class().tag()) {
			case cyng::TC_STRING:
				return to_line_protocol(cyng::io::to_str(obj));
				//return "\"" + cyng::io::to_str(obj) + "\"";
			case cyng::TC_INT8:
			case cyng::TC_INT16:
			case cyng::TC_INT32:
			case cyng::TC_INT64:
				return cyng::io::to_str(obj) + "i";
			case cyng::TC_TIME_POINT:
				return to_line_protocol(cyng::value_cast(obj, std::chrono::system_clock::now()));
			default:
				break;
			}
			return cyng::io::to_str(obj);
		}
		std::string to_line_protocol(std::string s)
		{
			//
			//	hex value
			//	test for [0..9a..f] 
			//	
			//
			if (std::all_of(s.begin(), s.end(), [](char c) {
				switch (c) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
				case 'A':
				case 'B':
				case 'C':
				case 'E':
				case 'D':
				case 'F':
					return true;
				default:
					break;
				}
				return false;
				})) {

				//
				//	hex value
				//
				try {
					auto const u = std::stoull(s, 0, 16);
					return std::to_string(u);
				}
				catch (std::exception const& ex) {
					;	//	continue
				}
			}

			//
			//	float/double value
			//	test for [0..9] and '.'
			//	
			//
			if (std::all_of(s.begin(), s.end(), [](char c) {
				switch (c) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case '.':
					return true;
				default:
					break;
				}
				return false;
				})) {

				//
				//	float/double value
				//

				return s;
			}

			//
			//	everything else
			//

			return "\"" + s + "\"";
		}


	}	//	sml
}	

