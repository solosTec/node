/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <nms/session.h>
#include <cache.h>
#include <cfg_rs485.h>
#include <cfg_wmbus.h>
#include <cfg_broker.h>
#include <cfg_redirector.h>
#include <cfg_nms.h>

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
#include <cyng/vector_cast.hpp>
#include <cyng/json.h>
#include <cyng/dom/algorithm.h>
#include <cyng/parser/version_parser.h>
#include <cyng/compatibility/file_system.hpp>
#include <cyng/sys/info.h>


namespace node
{
	namespace nms
	{
		session::session(cyng::async::mux& mux
			, boost::asio::ip::tcp::socket socket
			, cyng::logging::log_ptr logger
			, cache& cfg
			, std::string const& account
			, std::string const& pwd
			, std::size_t tsk)
		: socket_(std::move(socket))
			, logger_(logger)
			, reader_(mux, logger, cfg, account, pwd, tsk)
			, buffer_()
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
			CYNG_LOG_INFO(logger_, "new NMS session ["
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
						CYNG_LOG_TRACE(logger_, "NMS session received "
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

						//authorized_ = false;

					}
				});
		}

		void session::process_data(cyng::buffer_t&& data)
		{
			//
			//	insert new record into "_wMBusUplink" table
			//	
			CYNG_LOG_TRACE(logger_, "NMS session received " << std::string(data.begin(), data.end()));
			parser_.read(data.begin(), data.end());

			//
			//	update "sx" value of this session/device
			//
			sx_ += data.size();
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
				CYNG_LOG_DEBUG(logger_, "NMS send response: "
					<< str);
			}
			else {
				CYNG_LOG_WARNING(logger_, "NMS sending response failed: "
					<< ec.message());
			}
//#endif

		}

		reader::reader(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cache& cfg
			, std::string const& account
			, std::string const& pwd
			, std::size_t tsk)
		: mux_(mux)
			, logger_(logger)
			, cache_(cfg)
			, account_(account)
			, pwd_(pwd)
			, tsk_rebind_(tsk)
			, uuid_gen_()
			, uuid_rnd_()
		{}

		boost::uuids::uuid reader::get_tag(std::string const& str)
		{
			try {
				return uuid_gen_(str);
			}
			catch (std::exception const&) {
			}

			return uuid_rnd_();
		}

		cyng::param_map_t reader::run(cyng::param_map_t&& pm)
		{
			CYNG_LOG_DEBUG(logger_, "NMS request: " << cyng::io::to_type(pm));

			//
			//	read request and execute commands
			//
			auto const dom = cyng::make_reader(pm);
			auto const cmd = cyng::value_cast(dom.get("command"), "");
			auto const rv = cyng::parse_version(cyng::value_cast(dom.get("version"), "0.0"));

			//
			//	get source tag
			//
			auto const source = cyng::value_cast(dom.get("source"), "00000000-0000-0000-0000-000000000000");
			boost::uuids::string_generator uuid_gen;
			auto const tag = uuid_gen(source);

			CYNG_LOG_INFO(logger_, "NMS "
				<< rv.first
				<< " command: "
				<< cmd);

			//
			//	check version - only version 1.0 is valid
			//
			if (!rv.second || rv.first != protocol_version_) {
				return cyng::param_map_factory
					("command", cmd)
					("ec", "wrong version")
					("version", protocol_version_)
					("source", tag)
					;
			}

			//
			//	check credentials
			//
			auto const credentials = cyng::to_param_map(dom.get("credentials"));

			if (!check_credentials(cmd, credentials)) {
				return cyng::param_map_factory
					("command", cmd)
					("ec", "unknown account or invalid password")
					("version", protocol_version_)
					("source", tag)
					;
			}
			if (boost::algorithm::equals(cmd, "merge")
				|| boost::algorithm::equals(cmd, "serialset")
				|| boost::algorithm::equals(cmd, "serial-set")
				|| boost::algorithm::equals(cmd, "setserial")
				|| boost::algorithm::equals(cmd, "set-serial")) {
				return cmd_merge(cmd, tag, dom);
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
				return cmd_reboot(cmd, tag);
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

		bool reader::check_credentials(std::string const& cmd, cyng::param_map_t const& credentials)
		{
			if (credentials.size() == 2) {
				auto const user = credentials.find("user");
				auto const pwd = credentials.find("pwd");
				if (user != credentials.end() 
					&& pwd != credentials.end()
					&& boost::algorithm::equals(account_, cyng::value_cast(user->second, ""))
					&& boost::algorithm::equals(pwd_, cyng::value_cast(pwd->second, "")))
				{
					return true;
				}
			}
			return false;
		}


		cyng::param_map_t reader::cmd_merge(std::string const& cmd, boost::uuids::uuid tag, cyng::param_map_reader const& dom)
		{
			//auto const ports = cyng::to_param_map(dom.get("serial-port"));


			cyng::param_map_t pm = cyng::param_map_factory
				("command", cmd)
				("ec", "ok")
				("source", tag)
				("version", protocol_version_)
				("serial-port", cyng::param_map_factory()())		
				;

			cmd_merge_serial(pm, dom, cyng::to_param_map(dom.get("serial-port")));
			cmd_merge_nms(pm, cyng::to_param_map(dom.get("nms")));
			return pm;
		}

		void reader::cmd_merge_serial(cyng::param_map_t& pm, cyng::param_map_reader const& dom, cyng::param_map_t ports)
		{
			cfg_rs485 rs485(cache_);
			cfg_wmbus wmbus(cache_);
			cfg_broker broker(cache_);

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
							cmd_merge_broker(pm , dom, port, cyng::to_vector(param.second));
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
								broker.set_enabled(source::WIRED_LMN, param.second);
							}
							else {
								//	wireless M-Bus
								broker.set_enabled(source::WIRELESS_LMN, param.second);
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
							cmd_merge_listener(pm, dom, port, cyng::to_param_map(param.second));
							//cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("not-implemented-yet"));
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
						else if (boost::algorithm::equals(param.first, "loop")) {
							//
							//	supported only by RS485
							//
							if (port_id == cfg_rs485::port_idx) {
								//
								//	merge loop parameters
								//
								cmd_merge_loop(pm, dom, port, cyng::to_param_map(param.second));
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
								//	not supported
								//
								cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("error: not supported by RS-485 interface"));

							}
							else {

								//
								//	serial-port/PORT_NAME/max-readout-frequency
								//	seconds
								//
								wmbus.set_monitor(param.second);
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
								//	update the parameters enabled, list, and mode
								//
								auto const block_map = cyng::to_param_map(param.second);

								//
								//	enabled/disbaled
								//
								auto pos = block_map.find("enabled");
								if (pos != block_map.end()) {
									wmbus.set_blocklist_enabled(pos->second);
								}

								//
								//	mode
								//
								pos = block_map.find("mode");
								if (pos != block_map.end()) {
									auto const mode = cyng::value_cast(pos->second, "");
									if (boost::algorithm::equals(mode, "drop")) {
										wmbus.set_blocklist_drop_mode();
									}
									else if (boost::algorithm::equals(mode, "accept")) {
										wmbus.set_blocklist_accept_mode();
									}
									else {
										CYNG_LOG_WARNING(logger_, "undefined block mode: " << mode);
										cyng::merge(pm, { "serial-port", port.first, param.first, "mode" }, cyng::make_object("error: undefined block mode"));

									}
								}

								pos = block_map.find("list");
								if (pos != block_map.end()) {
									auto const list = cyng::vector_cast<std::string>(pos->second, "");
									CYNG_LOG_TRACE(logger_, "block list size: " << list.size());

									//
									//	convert into u32 values
									//
									std::vector<std::uint32_t> vec;
									std::transform(std::begin(list), std::end(list), std::back_inserter(vec), [](std::string id) -> std::uint32_t {
										try {
											return static_cast<std::uint32_t>(std::stoul(id, nullptr, 10));
										}
										catch (std::exception const&) {}
										return 0u;
										});
									wmbus.set_block_list(vec);
								}
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
		}

		void reader::cmd_merge_loop(cyng::param_map_t& pm, cyng::param_map_reader const& dom, cyng::param_t const& port, cyng::param_map_t params)
		{
			for (auto const& param : params) {
				if (boost::algorithm::equals(param.first, "timeout")) {
				}
				else if (boost::algorithm::equals(param.first, "request")) {
				}
				else {
					cyng::merge(pm, { port.first, "loop", param.first }, cyng::make_object("error: unknown loop attribute"));
				}
			}
		}

		void reader::cmd_merge_listener(cyng::param_map_t& pm, cyng::param_map_reader const& dom, cyng::param_t const& port, cyng::param_map_t params)
		{
			cfg_redirector redirector(cache_);

			for (auto const& param : params) {
				if (boost::algorithm::equals(param.first, "account")) {
					auto const account = cyng::value_cast(param.second, "");
					redirector.set_account(port.first, account);
				}
				else if (boost::algorithm::equals(param.first, "address")) {
					auto const address = cyng::value_cast(param.second, "");
					redirector.set_address(port.first, address);
				}
				else if (boost::algorithm::equals(param.first, "port")) {
					auto const service = cyng::numeric_cast<std::uint16_t>(param.second, 6006u);
					redirector.set_port(port.first, service);
				}
				else if (boost::algorithm::equals(param.first, "pwd")) {
					auto const pwd = cyng::value_cast(param.second, "");
					redirector.set_pwd(port.first, pwd);
				}
				else if (boost::algorithm::equals(param.first, "enabled")) {
					redirector.set_enabled(port.first, param.second);
				}
				else {
					cyng::merge(pm, { port.first, "listener", param.first }, cyng::make_object("error: unknown listener attribute"));
				}
			}
		}

		void reader::cmd_merge_broker(cyng::param_map_t& pm, cyng::param_map_reader const& dom, cyng::param_t const& port, cyng::vector_t vec)
		{
			cfg_broker broker(cache_);
			std::uint8_t idx{ 1 };
			for (auto const& obj : vec) {
				auto const sender = cyng::to_param_map(obj);
				for (auto const& p : sender) {
					CYNG_LOG_TRACE(logger_, "broker: " << p.first << ": " << cyng::io::to_type(p.second));
					if (boost::algorithm::equals(p.first, "address")) {
						auto const address = cyng::value_cast(p.second, "");
						broker.set_address(port.first, idx, address);
					}
					else if (boost::algorithm::equals(p.first, "port")) {
						auto const service = cyng::numeric_cast<std::uint16_t>(p.second, 12000u);
						broker.set_port(port.first, idx, service);
					}
					else if (boost::algorithm::equals(p.first, "account")) {
						auto const user = cyng::value_cast(p.second, "");
						broker.set_account(port.first, idx, user);
					}
					else if (boost::algorithm::equals(p.first, "pwd")) {
						auto const pwd = cyng::value_cast(p.second, "");
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
			//cyng::merge(pm, { "serial-port", port.first, param.first }, cyng::make_object("not-implemented-yet"));
		}

		void reader::cmd_merge_nms(cyng::param_map_t& pm, cyng::param_map_t params)
		{
			cfg_nms nms(cache_);

			for (auto const& param : params) {
				if (boost::algorithm::equals(param.first, "enabled")) {
					auto const enabled = cyng::value_cast(param.second, true);
					nms.set_enabled(true);
					if (!enabled)	cyng::merge(pm, { "nms", param.first }, cyng::make_object("warning: connection will be closed"));
				}
				else if (boost::algorithm::equals(param.first, "address")) {
					auto const address = cyng::make_address(cyng::value_cast(param.second, "0.0.0.0"));
					nms.set_address(address);
					cyng::merge(pm, { "nms", param.first }, cyng::make_object("info: restarts immediately"));
				}
				else if (boost::algorithm::equals(param.first, "port")) {
					auto const port = cyng::numeric_cast<std::uint16_t>(param.second, 7261);
					nms.set_port(port);
					cyng::merge(pm, { "nms", param.first }, cyng::make_object("info: restarts immediately"));
				}
				else if (boost::algorithm::equals(param.first, "pwd")) {
					auto const pwd = cyng::value_cast(param.second, "");
					nms.set_pwd(pwd);
				}
				else if (boost::algorithm::equals(param.first, "user")) {
					auto const user = cyng::value_cast(param.second, "");
					nms.set_user(user);
				}
				else {
					cyng::merge(pm, { "nms", param.first }, cyng::make_object("error: unknown NMS attribute"));
				}
			}

			if (!params.empty()) {
				mux_.post(tsk_rebind_, 0, cyng::tuple_factory({ nms.get_ep() }));
			}
		}

		cyng::param_map_t reader::cmd_query(std::string const& cmd, boost::uuids::uuid tag)
		{
			//
			//	serial ports
			//
			cfg_rs485 const rs485(cache_);
			cfg_wmbus const wmbus(cache_);
			cfg_broker const broker(cache_);
			cfg_redirector const redirector(cache_);
			cfg_nms const nms(cache_);

			return cyng::param_map_factory
				("command", cmd)
				("version", protocol_version_)
				("source", tag)
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
						("broker", broker.get_server_vector(source::WIRED_LMN))
						("listener", redirector.get_server_obj(source::WIRED_LMN))	//	could be null
						//("max-readout-frequency", 5)
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
						("broker", broker.get_server_vector(source::WIRELESS_LMN))
						("max-readout-frequency", wmbus.get_monitor().count())
						("blocklist", cyng::param_map_factory
							("enabled", wmbus.is_blocklist_enabled())
							("list", wmbus.get_block_list_vector())
							("mode", (wmbus.is_blocklist_drop_mode() ? "drop" : "accept"))
							())
						()),
					cyng::set_factory("nms", cyng::param_map_factory
						("enabled", nms.is_enabled())	//	this is redundant
						("address", nms.get_address())
						("port", nms.get_port())
						("user", nms.get_user())
						("pwd", nms.get_pwd())
						())
				))
				;
		}

		cyng::param_map_t reader::cmd_reboot(std::string const& cmd, boost::uuids::uuid tag)
		{
#if BOOST_OS_LINUX
#if defined(NODE_CROSS_COMPILE)

			auto const script_path = cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_NMS }, "script-path"), "/tmp/update-script.sh");

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
			("command", cmd)
				("ec", !ec ? "ok" : ec.message())
				("version", protocol_version_)
				("source", tag)
				("rc", "reboot scheduled")
				;
#else
			//	after 1 minute
			auto const rc = std::system("shutdown -r +1");
			return cyng::param_map_factory
			("command", cmd)
				("ec", "ok")
				("version", protocol_version_)
				("source", tag)
				("rc", rc)
				;

#endif
#else
			//	after 15 seconds
			auto const rc = std::system("shutdown /r /t 15");
			return cyng::param_map_factory
				("command", cmd)
				("ec", "ok")
				("version", protocol_version_)
				("source", tag)
				("rc", rc)
				;
#endif

		}

		cyng::param_map_t reader::cmd_update(std::string const& cmd, boost::uuids::uuid tag, cyng::param_map_reader const& dom)
		{
			auto const tmp = cyng::filesystem::temp_directory_path();
			auto const script_path = cache_.get_cfg(build_cfg_key({ sml::OBIS_ROOT_NMS }, "script-path"),
#if BOOST_OS_LINUX
			(tmp / "update-script.sh").string()
#else
			(tmp / "update-script.cmd").string()
#endif
			);

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
			else {
				ec = cyng::make_error_code(cyng::errc::file_exists);
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

		cyng::param_map_t reader::cmd_version(std::string const& cmd, boost::uuids::uuid tag)
		{
			return cyng::param_map_factory
				("command", cmd)
				("ec", "ok")
				("version", protocol_version_)
				("source", tag)
				("name", cyng::sys::get_os_name())
				("release", cyng::sys::get_os_release())
				;

		}

		cyng::param_map_t reader::cmd_cm(std::string const& cmd, boost::uuids::uuid tag, cyng::param_map_reader const& dom)
		{

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
			cyng::filesystem::path const script(script_path);
			cyng::error_code ec;

			//
			//	remove old file
			//
			cyng::filesystem::remove(script, ec);

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
				cyng::filesystem::permissions(script
					, cyng::filesystem::perms::owner_exec | cyng::filesystem::perms::group_exec | cyng::filesystem::perms::others_exec
					, cyng::filesystem::perm_options::add
					, ec);
			}
			else {
				ec = cyng::make_error_code(cyng::errc::file_exists);
			}



			return cyng::param_map_factory
				("command", cmd)
				("ec", (ec ? ec.message() : "ok"))
				("version", protocol_version_)
				("source", tag)
				("path", script_path)
				;
		}

		cyng::param_map_t reader::cmd_update_status(std::string const& cmd, boost::uuids::uuid tag, cyng::param_map_reader const& dom)
		{
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

			if (cyng::filesystem::exists(path)) {
				auto const tp = cyng::filesystem::get_write_time(path);

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
