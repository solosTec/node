/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "session.h"
#include "../cache.h"
#include "../cfg_rs485.h"
#include "../cfg_wmbus.h"

#include <smf/cluster/generator.h>
#include <smf/sml/srv_id_io.h>
#include <smf/serial/parity.h>
#include <smf/serial/stopbits.h>
#include <smf/serial/flow_control.h>

#include <cyng/io/io_bytes.hpp>
#include <cyng/io/io_buffer.h>
#include <cyng/io/hex_dump.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/controller.h>
#include <cyng/util/split.h>
#include <cyng/table/key.hpp>
#include <cyng/table/body.hpp>
#include <cyng/set_cast.h>
#include <cyng/dom/reader.h>
#include <cyng/json.h>


namespace node
{
	namespace nms
	{
		session::session(boost::asio::ip::tcp::socket socket
			, cyng::logging::log_ptr logger
			, cache& cfg)
		: socket_(std::move(socket))
			, logger_(logger)
			, reader_(logger, cfg)
			, buffer_()
			, authorized_(false)
			, data_()
			, rx_(0)
			, sx_(0)
			, parser_([&](cyng::vector_t&& data) {
				//CYNG_LOG_TRACE(logger_, cyng::io::to_str(data));
				for (auto const& obj : data) {

					//
					//	execute commands
					//	send response
					//
					this->send_response(reader_.run(cyng::to_param_map(obj)));

				}

			})
		{
			CYNG_LOG_INFO(logger_, "new session ["
				//<< vm_.tag()
				<< "] at "
				<< socket_.remote_endpoint());
		}

		session::~session()
		{
			//
			//	remove from session list
			//
		}

		void session::start()
		{
			do_read();
		}

		void session::do_read()
		{
			auto self(shared_from_this());

			socket_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
				[this, self](boost::system::error_code ec, std::size_t bytes_transferred)
				{
					if (!ec)
					{
						CYNG_LOG_TRACE(logger_, "session received "
							<< cyng::bytes_to_str(bytes_transferred));

#ifdef _DEBUG
						cyng::io::hex_dump hd;
						std::stringstream ss;
						hd(ss, buffer_.begin(), buffer_.begin() + bytes_transferred);
						CYNG_LOG_TRACE(logger_, "\n" << ss.str());
#endif
						//
						//	feeding the parser
						//
						process_data(cyng::buffer_t{ buffer_.begin(), buffer_.begin() + bytes_transferred });

						//
						//	continue reading
						//
						do_read();
					}
					else
					{
						//	leave
						CYNG_LOG_WARNING(logger_, "NMS session closed <" << ec << ':' << ec.value() << ':' << ec.message() << '>');

						//
						//	remove client
						//
						//cluster_.async_run(client_req_close(vm_.tag(), ec.value()));

						authorized_ = false;

					}
				});
		}

		void session::process_data(cyng::buffer_t&& data)
		{
			//
			//	insert new record into "_wMBusUplink" table
			//	
			parser_.read(data.begin(), data.end());

			//
			//	update "sx" value of this session/device
			//
			sx_ += data.size();
			//cluster_.async_run(bus_req_db_modify("_Session"
			//	, cyng::table::key_generator(vm_.tag())
			//	, cyng::param_factory("sx", sx_)
			//	, 0u
			//	, vm_.tag()));


		}

		void session::send_response(cyng::param_map_t&& res)
		{
			auto const str = cyng::json::to_string(res);

			boost::system::error_code ec;
			boost::asio::write(socket_, boost::asio::buffer(str.data(), str.size()), ec);

			if (!ec) {
				CYNG_LOG_DEBUG(logger_, "send response: "
					<< str);
			}
			else {
				CYNG_LOG_WARNING(logger_, "sending response failed: "
					<< ec.message());
			}
		}


		reader::reader(cyng::logging::log_ptr logger, cache& cfg)
			: logger_(logger)
			, cache_(cfg)
		{}

		cyng::param_map_t reader::run(cyng::param_map_t&& pm)
		{
			CYNG_LOG_DEBUG(logger_, "NMS request: " << cyng::io::to_type(pm));

			//
			//	read request and execute commands
			//
			auto const dom = cyng::make_reader(pm);
			auto const cmd = cyng::value_cast<std::string>(dom.get("command"), "");
			auto const ports = cyng::to_param_map(dom.get("serial-port"));
			auto const meter = cyng::to_param_map(dom.get("meter"));

			CYNG_LOG_INFO(logger_, "NMS command: " 
				<< cmd 
				<< " with " 
				<< ports.size() 
				<< " port(s)");

			if (boost::algorithm::equals(cmd, "delete")) {
				pm["ec"] = cyng::make_object("not implemented yet");
			}
			else if (boost::algorithm::equals(cmd, "insert")) {
				pm["ec"] = cyng::make_object("not implemented yet");
			}
			else if (boost::algorithm::equals(cmd, "merge")) {
				cmd_merge(pm, ports, meter);
				pm["ec"] = cyng::make_object("OK");
			}
			else if (boost::algorithm::equals(cmd, "query")) {
				return cmd_query();
			}
			else if (boost::algorithm::equals(cmd, "update")) {
				pm["ec"] = cyng::make_object("not implemented yet");
			}
			else if (boost::algorithm::equals(cmd, "reboot")) {
				cmd_reboot();
				pm["ec"] = cyng::make_object("not implemented yet");
			}
			else {
				CYNG_LOG_WARNING(logger_, "unknown NMS command: " << cmd);
				pm["ec"] = cyng::make_object("unknown NMS command: " + cmd);
			}

			//
			//	send response
			//
			return pm;
		}

		void reader::cmd_merge(cyng::param_map_t& pm, cyng::param_map_t const& ports, cyng::param_map_t const& meter)
		{
			for (auto const& port : ports) {
				CYNG_LOG_TRACE(logger_, "merge port: " << port.first);

			}

			for (auto const& section : meter) {
				CYNG_LOG_TRACE(logger_, "merge meter section: " << section.first);
			}

		}

		cyng::param_map_t reader::cmd_query()
		{
			//
			//	serial ports
			//
			cfg_rs485 const rs485(cache_);
			cfg_wmbus const wmbus(cache_);

			return cyng::param_map_factory
				("command", "query")
				("ec", "OK")
				("serial-port", cyng::tuple_factory(
					cyng::set_factory(rs485.get_port(), cyng::param_map_factory
						("enabled", rs485.is_enabled())
						("parity", serial::to_str(rs485.get_parity()))
						("databits", rs485.get_databits().value())
						("flow-control", serial::to_str(rs485.get_flow_control()))
						("stopbits", serial::to_str(rs485.get_stopbits()))
						("baudrate", rs485.get_baud_rate().value())
						("protocol", rs485.get_protocol_by_name())
						("broker", cyng::vector_factory())
						()),
					cyng::set_factory(wmbus.get_port(), cyng::param_map_factory
						("enabled", wmbus.is_enabled())
						("parity", serial::to_str(wmbus.get_parity()))
						("databits", wmbus.get_databits().value())
						("flow-control", serial::to_str(wmbus.get_flow_control()))
						("stopbits", serial::to_str(wmbus.get_stopbits()))
						("baudrate", wmbus.get_baud_rate().value())
						("broker", cyng::vector_factory())
						())
				))
				("meter", cyng::param_map_factory
					("blocklist", cyng::param_map_factory
						("enabled", false)
						("list", cyng::vector_factory())
						("mode", "drop")
						())
					("loop", cyng::param_map_factory
						("timeout", 60)
						("request", "/?!")
						())
					("max-readout-frequency", 5)
					())
				;

		}

		void reader::cmd_reboot()
		{

		}

	}
}
