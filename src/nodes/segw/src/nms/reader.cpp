/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <nms/reader.h>
#include <config/cfg_nms.h>
#include <config/cfg_broker.h>
#include <config/cfg_listener.h>
#include <config/cfg_blocklist.h>

#include <cyng/log/record.h>
#include <cyng/io/ostream.h>
#include <cyng/parse/version.h>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/sys/filesystem.h>
#include <cyng/parse/string.h>
#include <cyng/obj/algorithm/merge.h>

#include <filesystem>
#include <fstream>

#include <boost/algorithm/string.hpp>

namespace smf {
	namespace nms {

		reader::reader(cfg& c, cyng::logger logger)
			: cfg_(c)
			, logger_(logger)
			, sgen_()
		{}

		cyng::param_map_t reader::run(cyng::param_map_t&& pmap, std::function<void(boost::asio::ip::tcp::endpoint ep)> rebind) {

			auto const dom = cyng::make_reader(pmap);
			auto const cmd = cyng::value_cast(dom.get("command"), "");
			auto const version = cyng::to_version(cyng::value_cast(dom.get("version"), "0.0"));

			//
			//	get source tag
			//
			auto const source = cyng::value_cast(dom.get("source"), "00000000-0000-0000-0000-000000000000");
			auto const tag = sgen_(source);

			CYNG_LOG_INFO(logger_, "[NMS] "
				<< version
				<< " command ["
				<< cmd
				<< "]");

			//
			//	ToDo: check credentials
			//
			//auto const credentials = cyng::to_param_map(dom.get("credentials"));

			if (boost::algorithm::equals(cmd, "merge")
				|| boost::algorithm::equals(cmd, "serialset")
				|| boost::algorithm::equals(cmd, "serial-set")
				|| boost::algorithm::equals(cmd, "setserial")
				|| boost::algorithm::equals(cmd, "set-serial")) {
				return cmd_merge(cmd, tag, dom, rebind);
			}
			else if (boost::algorithm::equals(cmd, "query")
				|| boost::algorithm::equals(cmd, "serialget")
				|| boost::algorithm::equals(cmd, "serial-get")
				|| boost::algorithm::equals(cmd, "get serial")
				|| boost::algorithm::equals(cmd, "get-serial")) {
				return cmd_query(cmd, tag);
			}
			else if (boost::algorithm::equals(cmd, "update")
				|| boost::algorithm::equals(cmd, "fw-update")
				|| boost::algorithm::equals(cmd, "fwupdate")) {
				//
				//	software update
				//
				return cmd_update(cmd, tag, dom);
			}
			else if (boost::algorithm::equals(cmd, "update-status")
				|| boost::algorithm::equals(cmd, "get-update")
				|| boost::algorithm::equals(cmd, "fw-status")
				|| boost::algorithm::equals(cmd, "fwstatus")
				|| boost::algorithm::equals(cmd, "get-update-status")) {
				//
				//	update state
				//
				return cmd_update_status(cmd, tag, dom);
			}
			else if (boost::algorithm::equals(cmd, "reboot")
				|| boost::algorithm::equals(cmd, "restart")) {
				//return cmd_reboot(cmd, tag);
			}
			else if (boost::algorithm::equals(cmd, "fwversion")
				|| boost::algorithm::equals(cmd, "version")
				|| boost::algorithm::equals(cmd, "fw-version")) {
				return cmd_version(cmd, tag);
			}
			else if (boost::algorithm::equals(cmd, "cminfos")
				|| boost::algorithm::equals(cmd, "cminfo")
				|| boost::algorithm::equals(cmd, "cm-infos")
				|| boost::algorithm::equals(cmd, "infos-cm")) {
				return cmd_cm(cmd, tag, dom);
			}
			else {
				CYNG_LOG_WARNING(logger_, "unknown NMS command: " << cmd);
				return cyng::param_map_factory
					("command", cmd)
					("ec", "unknown command")
					("version", protocol_version_)
					("source", tag)
					;
			}

			//
			//	send response
			//
			return cyng::param_map_factory
				("ec", "command not specified")
				("version", protocol_version_)
				("source", tag)
				;

		}

		cyng::param_map_t reader::cmd_merge(std::string const& cmd, boost::uuids::uuid tag, pmap_reader const& dom, std::function<void(boost::asio::ip::tcp::endpoint ep)> rebind) {

			cyng::param_map_t pmap = cyng::param_map_factory
				("command", cmd)
				("ec", "ok")
				("source", tag)
				("version", protocol_version_)
				("serial-port", cyng::param_map_factory()())
				;

			cmd_merge_serial(pmap, cyng::container_cast<cyng::param_map_t>(dom["serial-port"].get()));
			cmd_merge_nms(pmap, cyng::container_cast<cyng::param_map_t>(dom["nms"].get()), rebind);

			return pmap;
		}

		void reader::cmd_merge_nms(cyng::param_map_t& pm, cyng::param_map_t&& params, std::function<void(boost::asio::ip::tcp::endpoint ep)> rebind) {

			cfg_nms cfg(cfg_);
			//CYNG_LOG_DEBUG(logger_, "[NMS] " << params);

			bool rebind_required = false;
			for (auto const& param : params) {
				if (boost::algorithm::equals(param.first, "enabled")) {
					auto const enabled = cyng::value_cast(param.second, true);
					cfg.set_enabled(enabled);
					if (!enabled)	cyng::merge(pm, { "nms", param.first }, cyng::make_object("warning: connection will be closed"));
				}
				else if (boost::algorithm::equals(param.first, "address")) {
					boost::system::error_code ec;
					auto const address = boost::asio::ip::make_address(cyng::value_cast(param.second, "0.0.0.0"), ec);
					cfg.set_address(address);
					cyng::merge(pm, { "nms", param.first }, cyng::make_object("info: restarts immediately"));
					rebind_required = true;
				}
				else if (boost::algorithm::equals(param.first, "port")) {
					auto const port = cyng::numeric_cast<std::uint16_t>(param.second, 7261);
					cfg.set_port(port);
					cyng::merge(pm, { "nms", param.first }, cyng::make_object("info: restarts immediately"));
					rebind_required = true;
				}
				else if (boost::algorithm::equals(param.first, "pwd")) {
					auto const pwd = cyng::value_cast(param.second, "");
					cfg.set_pwd(pwd);
				}
				else if (boost::algorithm::equals(param.first, "user")) {
					auto const user = cyng::value_cast(param.second, "");
					cfg.set_account(user);
				}
				else {
					cyng::merge(pm, { "nms", param.first }, cyng::make_object("error: unknown NMS attribute"));
				}
			}

			//
			//	rebind
			//
			if (rebind_required && rebind) {
				rebind(cfg.get_ep());
			}

		}

		void reader::cmd_merge_serial(cyng::param_map_t& pm, cyng::param_map_t&& ports) {

			for (auto const& port : ports) {
				CYNG_LOG_DEBUG(logger_, "[NMS] merge port " << port.first << " = " << port.second);
				cfg_lmn cfg(cfg_, port.first);


				auto const port_pmap = cyng::container_cast<cyng::param_map_t>(port.second);
				for (auto const& param : port_pmap) {
					if (boost::algorithm::equals(param.first, "enabled")) {
						auto const enabled = cyng::value_cast(param.second, true);
						cfg.set_enabled(enabled);
						cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
					}
					else if (boost::algorithm::equals(param.first, "parity")) {
						auto const parity = cyng::value_cast(param.second, "none");
						cfg.set_parity(parity);
						cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
					}
					else if (boost::algorithm::equals(param.first, "databits")) {
						auto const databits = cyng::numeric_cast<std::uint8_t>(param.second, 8);
						cfg.set_databits(databits);
						cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
					}
					else if (boost::algorithm::equals(param.first, "flow-control")) {
						auto const flow_control = cyng::value_cast(param.second, "none");
						cfg.set_flow_control(flow_control);
						cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
					}
					else if (boost::algorithm::equals(param.first, "stopbits")) {
						auto const stopbits = cyng::value_cast(param.second, "one");
						cfg.set_stopbits(stopbits);
						cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
					}
					else if (boost::algorithm::equals(param.first, "baudrate")) {
						auto const baudrate = cyng::numeric_cast<std::uint32_t>(param.second, 2400);
						cfg.set_baud_rate(baudrate);
						cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
					}
					else if (boost::algorithm::equals(param.first, "protocol")) {
						auto const protocol = cyng::value_cast(param.second, "auto");
						cfg.set_protocol(protocol);
						cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("ok"));
					}
					else if (boost::algorithm::equals(param.first, "broker")) {
						cmd_merge_broker(pm, cfg.get_lmn_type(), cyng::container_cast<cyng::vector_t>(param.second));
					}
					else if (boost::algorithm::equals(param.first, "listener")) {
					}
					else {
						cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("error: unknown attribute"));
					}
				}
			}
		}

		void reader::cmd_merge_broker(cyng::param_map_t& pm, lmn_type type, cyng::vector_t&& vec) {

			cfg_broker cfg(cfg_, type);

			//
			//	ToDo: remove old entries
			//

			std::size_t idx{ 1 };	//	broker index
			for (auto const& obj : vec) {
				auto const pmap = cyng::container_cast<cyng::param_map_t>(obj);
				for (auto const& broker : pmap) {
					if (boost::algorithm::equals(broker.first, "address")) {
						auto const address = cyng::value_cast(broker.second, "");
						cfg.set_address(idx, address);
					}
					else if (boost::algorithm::equals(broker.first, "port")) {
						auto const port = cyng::numeric_cast<std::uint16_t>(broker.second, 12000u);
						cfg.set_port(idx, port);
					}
					else if (boost::algorithm::equals(broker.first, "account")) {
						auto const account = cyng::value_cast(broker.second, "");
						cfg.set_account(idx, account);
					}
					else if (boost::algorithm::equals(broker.first, "pwd")) {
						auto const pwd = cyng::value_cast(broker.second, "");
						cfg.set_pwd(idx, pwd);
					}
					else {
						CYNG_LOG_WARNING(logger_, "[NMS] unknown broker attribute: " << broker.first);
					}
				}
				idx++;
			}

			BOOST_ASSERT(idx == vec.size());
			cfg.set_size(idx);
		}

		cyng::param_map_t reader::cmd_query(std::string const& cmd, boost::uuids::uuid tag) {

			cfg_lmn wmbus(cfg_, lmn_type::WIRELESS);
			cfg_lmn rs485(cfg_, lmn_type::WIRED);
			cfg_nms cfg(cfg_);
			cfg_broker wmbus_broker(cfg_, lmn_type::WIRELESS);
			cfg_broker rs485_broker(cfg_, lmn_type::WIRED);
			//cfg_listener wmbus_listener(cfg_, lmn_type::WIRELESS);
			cfg_listener rs485_listener(cfg_, lmn_type::WIRED);
			cfg_blocklist wmbus_blocklist(cfg_, lmn_type::WIRELESS);
			//cfg_blocklist rs485_blocklist(cfg_, lmn_type::WIRED);

			return cyng::param_map_factory
				("command", cmd)
				("version", protocol_version_)
				("source", tag)
				("ec", "ok")
				("serial-port", cyng::tuple_factory(
					cyng::make_param(rs485.get_port(), cyng::param_map_factory
						("enabled", rs485.is_enabled())
						("index", rs485.get_index())
						("parity", serial::to_str(rs485.get_parity()))
						("databits", rs485.get_databits().value())
						("flow-control", serial::to_str(rs485.get_flow_control()))
						("stopbits", serial::to_str(rs485.get_stopbits()))
						("baudrate", rs485.get_baud_rate().value())
						("protocol", rs485.get_protocol())
						("broker", rs485_broker.get_target_vector())
						("listener", cyng::param_map_factory
							("enabled", rs485_listener.is_enabled())
							("address", rs485_listener.get_address())
							("port", rs485_listener.get_port())
							())
						("loop", cyng::param_map_factory
							("timeout", 60)
							("request", "/?!")
							())
						()),
					cyng::make_param(wmbus.get_port(), cyng::param_map_factory
						("enabled", wmbus.is_enabled())
						("index", wmbus.get_index())
						("parity", serial::to_str(wmbus.get_parity()))
						("databits", wmbus.get_databits().value())
						("flow-control", serial::to_str(wmbus.get_flow_control()))
						("stopbits", serial::to_str(wmbus.get_stopbits()))
						("baudrate", wmbus.get_baud_rate().value())
						("protocol", wmbus.get_protocol())
						("broker", wmbus_broker.get_target_vector())
						("blocklist", cyng::param_map_factory
							("enabled", wmbus_blocklist.is_enabled())
							("mode", wmbus_blocklist.get_mode())
							("list", wmbus_blocklist.get_list())
							()
						)
						()),
					cyng::make_param("nms", cyng::param_map_factory
						("enabled", true)	//	this is redundant
				//		("enabled", nms.is_enabled())	//	this is redundant
						("address", cfg.get_address())
						("port", cfg.get_port())
						("user", cfg.get_account())
						("pwd", cfg.get_pwd())
						("debug", cfg.is_debug())
						())
				))
				;
		}

		cyng::param_map_t reader::cmd_update(std::string const& cmd, boost::uuids::uuid tag, pmap_reader const& dom) {

			cfg_nms cfg(cfg_);
			auto const script_path = cfg.get_script_path();

			auto const address = cyng::value_cast(dom.get("address"), "localhost");
			auto const port = cyng::numeric_cast<std::int16_t>(dom.get("port"), 9009);
			auto const username = cyng::value_cast(dom.get("user"), "user");
			auto const password = cyng::value_cast(dom.get("pwd"), "pwd");
			auto const filename = cyng::value_cast(dom.get("fw-filename"), "fw-filname");
			auto const ca_path_download = cyng::value_cast(dom.get("download-ca-path"), "");
			auto const ca_path_vendor = cyng::value_cast(dom.get("vendor-ca-path"), "");
			auto const path_firmware = cyng::value_cast(dom.get("firmware-path"), "");

			//
			//	create a script file
			//
			std::filesystem::path const script(script_path);
			std::error_code ec;

			//
			//	remove old file
			//
			std::filesystem::remove(script, ec);

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
				std::filesystem::permissions(script
					, std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec
					, std::filesystem::perm_options::add
					, ec);
			}
			else {
				ec = std::make_error_code(std::errc::file_exists);
			}

			return cyng::param_map_factory
				("command", cmd)
				("ec", !ec ? "ok" : ec.message())
				("version", protocol_version_)
				("source", tag)
				("script-path", script_path)
				("address", address)
				("port", port)
				("user", username)
				("fw-filename", filename)
				("download-ca-path", ca_path_download)
				("vendor-ca-path", ca_path_vendor)
				("firmware-path", path_firmware)
				;
		}

		cyng::param_map_t reader::cmd_update_status(std::string const& cmd, boost::uuids::uuid tag, pmap_reader const& dom) {
			//
			//	read file "/usr/local/CLS/etc/firmwareupdate.conf"
			//
			std::filesystem::path const path(
#if BOOST_OS_LINUX
				"/usr/local/CLS/etc/firmwareupdate.conf"
#else
				"firmwareupdate.conf"
#endif
			);

			if (std::filesystem::exists(path)) {
				auto const tp = cyng::sys::get_write_time(path);

				std::ifstream fs(path.string());
				if (fs.is_open()) {

					//
					//	read line
					//
					std::string line;
					if (std::getline(fs, line)) {

						//
						//	format: fwUpdateStatus=N
						//
						auto const vec = cyng::split(line, "=");
						if (vec.size() == 2) {

							try {
								int code = std::stoi(vec.at(1));

								return cyng::param_map_factory
									("command", cmd)
									("ec", "ok")
									("version", protocol_version_)
									("source", tag)
									("path", path)
									("line", line)
									("code", code)
									("msg", get_code_name(code))
									("modified", tp)
									;
							}
							catch (std::exception const& ex) {
								return cyng::param_map_factory
									("command", cmd)
									("ec", ex.what())
									("version", protocol_version_)
									("source", tag)
									("path", path)
									("line", line)
									("code", -99)
									("modified", tp)
									;
							}
						}
						else {
							return cyng::param_map_factory
								("command", cmd)
								("ec", "error: invalid format")
								("version", protocol_version_)
								("source", tag)
								("path", path)
								("line", line)
								("code", -99)
								("modified", tp)
								;
						}
					}
				}
			}


			return cyng::param_map_factory
				("command", cmd)
				("ec", "error: file not found")
				("version", protocol_version_)
				("source", tag)
				("path", path)
				("code", -99)
				;
		}

		cyng::param_map_t reader::cmd_version(std::string const& cmd, boost::uuids::uuid tag) {

			return cyng::param_map_factory
				("command", cmd)
				("ec", "ok")
				("version", protocol_version_)
				("source", tag)
				//("name", cyng::sys::get_os_name())
				//("release", cyng::sys::get_os_release())
				;

		}

		cyng::param_map_t reader::cmd_cm(std::string const& cmd, boost::uuids::uuid tag, pmap_reader const& dom) {

			//
			//	create a script file
			//
			std::string const script_path(
#if BOOST_OS_LINUX
				"/usr/local/etc/cminfos.txt"
#else
				"cminfos.txt"
#endif
			);
			std::filesystem::path const script(script_path);
			std::error_code ec;

			//
			//	remove old file
			//
			std::filesystem::remove(script, ec);

			std::fstream fs(script_path, std::fstream::trunc | std::fstream::out);
			if (fs.is_open()) {

				fs
					<< "cm-id = "
					<< cyng::value_cast(dom.get("cm-id"), "000000000000")
					<< std::endl
					<< "dns-ip = "
					<< cyng::value_cast(dom.get("dns-ip"), "0.0.0.0")
					<< std::endl
					;
				fs.close();

				//
				//	set permissions
				//
				std::filesystem::permissions(script
					, std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec
					, std::filesystem::perm_options::add
					, ec);
			}
			else {
				ec = std::make_error_code(std::errc::file_exists);
			}



			return cyng::param_map_factory
				("command", cmd)
				("ec", (ec ? ec.message() : "ok"))
				("version", protocol_version_)
				("source", tag)
				("path", script_path)
				;
		}

		std::string get_code_name(int status)
		{
			switch (status)
			{
			case -99:   return "UNDEFINED";
			case 0:     return "SUCCESS";
			case 1:     return "DOWNLOADING";   //  Firmware Download Procedure Initiated Successfully
			case 2:     return "VERIFIED";      //  Firmware Verification Procedure Initiated Successfully
			case 3:     return "INITIATED";     //  Firmware Update Procedure Initiated Successfully
			case -1:    return "CURL-ERROR-GENERAL";    //    Error while executing curl command
			case -2:    return "INVALID-SIGNATURE"; //    Couldn't verify the signatures, aborting FW update
			case -6:    return "ABORTED";       //   Error while initiating FW Update, aborting FW update

			case 101:   return "PARAMETER-ERROR-DOWNLOAD";  //  Parameter error in FW Download
			case 102:   return "ROOT-CA-NOT-FOUND-1"; //   Path to Root CA not found
			case 103:   return "CERTSOUT-NOT-FOUND-1"; //   Path to Certsout not found
			case 104:   return "HTTP-DOWNLOAD-FAILED"; //   HTTP error as download failed
			case 105:   return "CURL-ERROR-DOWNLOAD"; //   Curl error as download failed

			case 201:   return "PARAMETER-ERROR-VERIFICATION"; //   Parameter Error in FW Verification
			case 202:   return "OPENSSL-TOOL-NOT-FOUND"; //   Openssl tool not found
			case 203:   return "CERT-FORMAT-NOT-SUPPORTED"; //   Format (DER/PEM) not supported
			case 204:   return "ROOT-CA-NOT-FOUND-2"; //   Path to Root CA not found
			case 205:   return "SIGNATURES-NOT-FOUND"; //   Signatures not found
			case 206:   return "FW-FILE-NOT-FOUND"; //   FW File not found
			case 207:   return "CERTSOUT-NOT-FOUND-2"; //   Path to Certsout not found
			case 209:   return "INVALID-COMMAND-OPTIONS"; //   an error occurred parsing the command options
			case 210:   return "CANNOT-READ-INPUT-FILE"; //   one of the input files could not be read
			case 211:   return "SIGNATURE-VERIFICATION-FAILED"; //   Signature verification ERROR (an error occurred creating the CMS file or when reading the MIME message)
			case 212:   return "CANNOT-DECRYPT-MESSAGE"; //   an error occurred decrypting or verifying the message
			case 213:   return "CANNOT-WRITE-OUT-SIGNERS-CERTIFICATE"; //   the message was verified correctly but an error occurred writing out the signers certificates
			case 214:   return "UNKNOWN-ERROR"; //   unknown error

			case 250:   return "ALREADY-RUNNING"; //   Script is already running
			default:
				break;
			}

			return "OTHER-ERROR";
		}


	}
}