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
#include "../cfg_broker.h"

#include <smf/cluster/generator.h>
#include <smf/sml/srv_id_io.h>
#include <smf/serial/parity.h>
#include <smf/serial/stopbits.h>
#include <smf/serial/flow_control.h>
#include <smf/sml/obis_db.h>

#include <cyng/io/io_bytes.hpp>
#include <cyng/io/io_buffer.h>
#include <cyng/io/hex_dump.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/controller.h>
#include <cyng/util/split.h>
#include <cyng/table/key.hpp>
#include <cyng/table/body.hpp>
#include <cyng/set_cast.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/json.h>
#include <cyng/dom/algorithm.h>
#include <cyng/parser/version_parser.h>
#include <cyng/compatibility/file_system.hpp>

//#include <boost/process.hpp>

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

//#ifdef _DEBUG
//			std::stringstream ss;
//			cyng::json::pretty_print(ss, cyng::make_object(res));
//			CYNG_LOG_DEBUG(logger_, "send response: " << ec << ss.str());
//#else
			if (!ec) {
				CYNG_LOG_DEBUG(logger_, "send response: "
					<< str);
			}
			else {
				CYNG_LOG_WARNING(logger_, "sending response failed: "
					<< ec.message());
			}
//#endif

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
			auto const rv = cyng::parse_version(cyng::value_cast<std::string>(dom.get("version"), ""));

			CYNG_LOG_INFO(logger_, "NMS "
				<< rv.first
				<< " command: "
				<< cmd);

			//
			//	check version
			//
			if (!rv.second || rv.first != cyng::version(0,1)) {
				return cyng::param_map_factory
					("command", cmd)
					("ec", "wrong version")
					("version", "0.1")
					;
			}
			else if (boost::algorithm::equals(cmd, "merge")) {
				return cmd_merge(dom);
			}
			else if (boost::algorithm::equals(cmd, "query")) {
				return cmd_query();
			}
			else if (boost::algorithm::equals(cmd, "update")) {
				//
				//	software update
				//
				return cmd_update(dom);
			}
			else if (boost::algorithm::equals(cmd, "reboot")) {
				return cmd_reboot();
			}
			else {
				CYNG_LOG_WARNING(logger_, "unknown NMS command: " << cmd);
				return cyng::param_map_factory
					("command", cmd)
					("ec", "unknown command")
					("version", "0.1")
					;

			}

			//
			//	send response
			//
			return cyng::param_map_factory
				("ec", "command not specified")
				("version", "0.1")
				;
		}

		cyng::param_map_t reader::cmd_merge(cyng::param_map_reader const& dom)
		{
			auto const ports = cyng::to_param_map(dom.get("serial-port"));
			auto const version = cyng::to_param_map(dom.get("version"));

			cfg_rs485 rs485(cache_);
			cfg_wmbus wmbus(cache_);
			cfg_broker broker(cache_);

			cyng::param_map_t pm = cyng::param_map_factory
				("command", "merge")
				("ec", "ok")
				("version", "0.1")
				("serial-port", cyng::param_map_factory()())
				("meter", cyng::param_map_factory()())
				
				;


			for (auto const& port : ports) {

				auto const port_id = broker.get_port_id(port.first);
				if (port_id != 0) {

					CYNG_LOG_TRACE(logger_, "merge port: " << port.first);
					cyng::merge(pm, { "serial-port", port.first, "index" }, cyng::make_object(port_id));

					auto const port_map = cyng::to_param_map(dom["serial-port"].get(port.first));
					for (auto const& param : port_map) {
						CYNG_LOG_TRACE(logger_, "merge port: "
							<< port.first
							<< " - "
							<< param.first
							<< ": "
							<< cyng::io::to_type(param.second));

						if (boost::algorithm::equals(param.first, "baudrate")) {
							//  merge port: /dev/ttyAPP0 - baudrate: "2400"
							if (port_id == cfg_rs485::port_idx) {
								rs485.set_baud_rate(param.second);
							}
							else {
								wmbus.set_baud_rate(param.second);
							}
							cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
						}
						else if (boost::algorithm::equals(param.first, "broker")) {
							//  merge port: /dev/ttyAPP0 - broker: [%(("account":"C4vvQP"),("address":"segw.ch"),("port":12001i64),("pwd":"9BLPJfNc"))]
							auto const vec = cyng::to_vector(param.second);
							std::uint8_t idx{ 1 };
							for (auto const& obj : vec) {
								auto const sender = cyng::to_param_map(obj);
								for (auto const& p : sender) {
									CYNG_LOG_TRACE(logger_, "broker: " << p.first << ": " << cyng::io::to_type(p.second));
									if (boost::algorithm::equals(p.first, "address")) {
										auto const address = cyng::value_cast<std::string>(p.second, "");
										broker.set_address(port.first, idx, address);
									}
									else if (boost::algorithm::equals(p.first, "port")) {
										auto const service = cyng::numeric_cast<std::uint16_t>(p.second, 12000u);
										broker.set_port(port.first, idx, service);
									}
									else if (boost::algorithm::equals(p.first, "account")) {
										auto const user = cyng::value_cast<std::string>(p.second, "");
										broker.set_account(port.first, idx, user);
									}
									else if (boost::algorithm::equals(p.first, "pwd")) {
										auto const pwd = cyng::value_cast<std::string>(p.second, "");
										broker.set_pwd(port.first, idx, pwd);
									}
									else {
										CYNG_LOG_WARNING(logger_, "merge port: "
											<< port.first
											<< " unknown attribute: "
											<< p.first
											<< ": "
											<< cyng::io::to_type(p.second));
									}
								}
								++idx;
							}
							cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("not-implemented-yet"));
						}
						else if (boost::algorithm::equals(param.first, "collector-login")) {
							//  merge port: /dev/ttyAPP0 - collector-login: true
							broker.set_login_required(port.first, cyng::value_cast(param.second, false));
							cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
						}
						else if (boost::algorithm::equals(param.first, "databits")) {
							//  merge port: /dev/ttyAPP0 - databits: 8i64
							if (port_id == cfg_rs485::port_idx) {
								rs485.set_databits(param.second);
							}
							else {
								wmbus.set_databits(param.second);
							}
							cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
						}
						else if (boost::algorithm::equals(param.first, "enabled")) {
							//  merge port: /dev/ttyAPP0 - enabled: true
							if (port_id == cfg_rs485::port_idx) {
								//	RS 485
								broker.set_enabled(cfg_broker::source::WIRED_LMN, param.second);
							}
							else {
								//	wireless M-Bus
								broker.set_enabled(cfg_broker::source::WIRELESS_LMN, param.second);
							}
							cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
						}
						else if (boost::algorithm::equals(param.first, "flow-control")) {
							//  merge port: /dev/ttyAPP0 - flow-control: "none"
							if (port_id == cfg_rs485::port_idx) {
								rs485.set_flow_control(param.second);
							}
							else {
								wmbus.set_flow_control(param.second);
							}
							cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
						}
						else if (boost::algorithm::equals(param.first, "listener")) {
							//  merge port: /dev/ttyAPP0 - listener: %(("address":"0.0.0.0"),("port":7000i64),("timeout":30i64))
							cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("not-implemented-yet"));
						}
						else if (boost::algorithm::equals(param.first, "parity")) {
							//  merge port: /dev/ttyAPP0 - parity: "even"
							if (port_id == cfg_rs485::port_idx) {
								rs485.set_parity(param.second);
							}
							else {
								wmbus.set_parity(param.second);
							}
							cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
						}
						else if (boost::algorithm::equals(param.first, "protocol")) {
							//  merge port: /dev/ttyAPP0 - protocol: "SML"
							if (port_id == cfg_rs485::port_idx) {
								rs485.set_protocol(param.second);
								cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
							}
							else {
								cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("error: not available"));
							}
						}
						else if (boost::algorithm::equals(param.first, "stopbits")) {
							//  merge port: /dev/ttyAPP0 - stopbits: "one"
							if (port_id == cfg_rs485::port_idx) {
								rs485.set_stopbits(param.second);
							}
							else {
								wmbus.set_stopbits(param.second);
							}
							cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
						}
						else if (boost::algorithm::equals(param.first, "loop"))	{
							//
							//	supported only by RS485
							//
							if (port_id == cfg_rs485::port_idx) {
								//
								//	ToDo
								//
								cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ToDo"));
							}
							else {
								cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("error: not supported by wirless M-Bus"));
							}
						}
						else if (boost::algorithm::equals(param.first, "max-readout-frequency")) {
							//
							//	supported only by RS485
							//
							if (port_id == cfg_rs485::port_idx) {
								//
								//	ToDo
								//
								cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ToDo"));

							}
							else {
								cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("error: not supported by wirless M-Bus"));
							}
						}
						else if (boost::algorithm::equals(param.first, "blocklist")) {
							//
							//	supported only by wireless M-Bus
							//
							if (port_id == cfg_rs485::port_idx) {
								cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("error: not supported by RS485"));
							}
							else {
								//
								//	ToDo
								//
								cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ToDo"));
							}
						}
						else {
							CYNG_LOG_WARNING(logger_, "merge port: "
								<< port.first
								<< " unknown attribute: "
								<< param.first
								<< ": "
								<< cyng::io::to_type(param.second));

							cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("error: unknown attribute"));
						}
					}
				}
				else {
					CYNG_LOG_WARNING(logger_, port.first << " is an undefined port");
					cyng::merge(pm, { "serial-port", port.first }, cyng::make_object("error: unknown hardware port"));
				}
			}

			//for (auto const& section : meter) {
			//	CYNG_LOG_TRACE(logger_, "merge meter section: " << section.first);
			//	//  merge meter section: blocklist
			//	//  merge meter section: loop
			//	//  merge meter section: max-readout-frequency
			//}

			return pm;
		}

		cyng::param_map_t reader::cmd_query()
		{
			//
			//	serial ports
			//
			cfg_rs485 const rs485(cache_);
			cfg_wmbus const wmbus(cache_);
			cfg_broker const broker(cache_);

			return cyng::param_map_factory
				("command", "query")
				("version", "0.1")
				("ec", "ok")
				("serial-port", cyng::tuple_factory(
					cyng::set_factory(rs485.get_port(), cyng::param_map_factory
						("enabled", rs485.is_enabled())
						("index", rs485.port_idx)
						("parity", serial::to_str(rs485.get_parity()))
						("databits", rs485.get_databits().value())
						("flow-control", serial::to_str(rs485.get_flow_control()))
						("stopbits", serial::to_str(rs485.get_stopbits()))
						("baudrate", rs485.get_baud_rate().value())
						("protocol", rs485.get_protocol_by_name())
						("broker", broker.get_broker_vector(cfg_broker::source::WIRED_LMN))
						("max-readout-frequency", 5)
						("loop", cyng::param_map_factory
							("timeout", 60)
							("request", "/?!")
							())
						()),
					cyng::set_factory(wmbus.get_port(), cyng::param_map_factory
						("enabled", wmbus.is_enabled())
						("index", wmbus.port_idx)
						("parity", serial::to_str(wmbus.get_parity()))
						("databits", wmbus.get_databits().value())
						("flow-control", serial::to_str(wmbus.get_flow_control()))
						("stopbits", serial::to_str(wmbus.get_stopbits()))
						("baudrate", wmbus.get_baud_rate().value())
						("broker", broker.get_broker_vector(cfg_broker::source::WIRELESS_LMN))
						("blocklist", cyng::param_map_factory
							("enabled", false)
							("list", cyng::vector_factory({ "00684279" }))
							("mode", "drop")
							())
						())
				))
				;
		}

		cyng::param_map_t reader::cmd_reboot()
		{
#if defined(NODE_CROSS_COMPILE) && BOOST_OS_LINUX

			auto const script_path = cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_NMS }, "script-path"),
#if BOOST_OS_LINUX
				"update-script.sh"
#else
				"update-script.cmd"
#endif
			);

			cyng::filesystem::path const script(script_path);
			cyng::error_code ec;

			//
			//	remove old file
			//
			cyng::filesystem::remove(script, ec);

			std::fstream fs(script_path, std::fstream::trunc | std::fstream::out);
			if (fs.is_open()) {
				fs
					<< "#!/bin/bash"
					<< std::endl
					<< "reboot.sh"
					<< std::endl
					;
				fs.close();

				//
				//	set permissions
				//
				cyng::filesystem::permissions(script
					, cyng::filesystem::perms::owner_exec | cyng::filesystem::perms::group_exec | cyng::filesystem::perms::others_exec
					, cyng::filesystem::perm_options::add
					, ec);
			}



			return cyng::param_map_factory
				("command", "reboot")
				("ec", !ec ? "ok" : ec.message())
				("version", "0.1")
				("rc", "reboot scheduled")
				;

#elif BOOST_OS_LINUX
			//	after 1 minute
			auto const rc = std::system("shutdown -r +1");
#else
			//	after 15 seconds
			auto const rc = std::system("shutdown /r /t 15");
#endif
			return cyng::param_map_factory
				("command", "reboot")
				("ec", "ok")
				("version", "0.1")
				("rc", rc)
				;

		}

		cyng::param_map_t reader::cmd_update(cyng::param_map_reader const& dom)
		{
			auto const script_path = cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_NMS }, "script-path"),
#if BOOST_OS_LINUX
				"update-script.sh"
#else
				"update-script.cmd"
#endif
			);

			auto const address = cyng::value_cast<std::string>(dom.get("address"), "");
			auto const port = cyng::value_cast<std::string>(dom.get("port"), "");
			auto const username = cyng::value_cast<std::string>(dom.get("username"), "");
			auto const password = cyng::value_cast<std::string>(dom.get("password"), "");
			auto const filename = cyng::value_cast<std::string>(dom.get("filename"), "fw-filname");
			auto const ca_path_download = cyng::value_cast<std::string>(dom.get("ca-path-download"), "");
			auto const ca_path_vendor = cyng::value_cast<std::string>(dom.get("ca-path-vendor"), "");
			auto const path_firmware = cyng::value_cast<std::string>(dom.get("path-firmware"), "");

			//
			//	create a script file
			//
			cyng::filesystem::path const script(script_path);
			cyng::error_code ec;

			//
			//	remove old file
			//
			cyng::filesystem::remove(script, ec);

			std::fstream fs(script_path, std::fstream::trunc | std::fstream::out);
			if (fs.is_open()) {
				fs
					<< "#!/bin/bash"
					<< std::endl
					<< "/usr/local/sbin/fw-update.sh "
					<< address 
					<< " \""
					<< username
					<< "\" \""
					<< password
					<< "\" \""
					<< filename
					<< "\" \""
					<< ca_path_download
					<< "\" \""
					<< ca_path_vendor
					<< "\" \""
					<< path_firmware
					<< "\" "
					<< port
					<< std::endl
					;
				fs.close();

				//
				//	set permissions
				//
				cyng::filesystem::permissions(script
					, cyng::filesystem::perms::owner_exec | cyng::filesystem::perms::group_exec | cyng::filesystem::perms::others_exec
					, cyng::filesystem::perm_options::add
					, ec);
			}


			return cyng::param_map_factory
			("command", "update")
				("ec", !ec ? "ok" : ec.message())
				("version", "0.1")
				("script-path", script_path)
				("address", address)
				("port", port)
				("username", username)
				("filename", filename)
				("ca-path-download", ca_path_download)
				("ca-path-vendor", ca_path_vendor)
				("path-firmware", path_firmware)
				;

		}

	}
}
