/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <ipt_session.h>

#include <smf/ipt/codes.h>
#include <smf/ipt/response.hpp>
#include <smf/ipt/transpiler.h>
#include <smf/ipt/query.h>

#include <cyng/log/record.h>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/algorithm/reader.hpp>

#include <iostream>

#ifdef _DEBUG_IPT
#include <iostream>
#include <sstream>
#include <cyng/io/hex_dump.hpp>
#endif

namespace smf {

	ipt_session::ipt_session(boost::asio::ip::tcp::socket socket
		, bus& cluster_bus
		, cyng::mesh& fabric
		, ipt::scramble_key const& sk
		, std::uint32_t query
		, cyng::logger logger)
	: socket_(std::move(socket))
		, logger_(logger)
		, cluster_bus_(cluster_bus)
		, query_(query)
		, buffer_()
		, buffer_write_()
		, parser_(sk
			, std::bind(&ipt_session::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&ipt_session::ipt_stream, this, std::placeholders::_1)
		)
		, serializer_(sk)
		, vm_()
		, dev_(boost::uuids::nil_uuid())
	{
		vm_ = fabric.create_proxy(cluster_bus_.get_tag()
			, get_vm_func_pty_res_login(this)
			, get_vm_func_pty_res_register(this)
			, get_vm_func_pty_res_open_channel(this));

		std::size_t slot{ 0 };
		vm_.set_channel_name("pty.res.login", slot++);
		vm_.set_channel_name("pty.res.register", slot++);
		vm_.set_channel_name("pty.res.open.channel", slot++);
		vm_.set_channel_name("pty.res.close.channel", slot++);

		CYNG_LOG_INFO(logger_, "[session] " << vm_.get_tag() << '@' << socket_.remote_endpoint() << " created");

	}

	ipt_session::~ipt_session()
	{
#ifdef _DEBUG_IPT
		std::cout << "session(~)" << std::endl;
#endif
	}

	void ipt_session::stop()
	{
		//	https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload2.html
		CYNG_LOG_WARNING(logger_, "[session] " << vm_.get_tag() << " stopped");
		boost::system::error_code ec;
		socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
		socket_.close(ec);

		vm_.stop();
	}

	void ipt_session::logout() {
		cluster_bus_.pty_logout(dev_, vm_.get_tag());
	}


	void ipt_session::start()
	{
		do_read();
	}

	void ipt_session::do_read() {
		auto self = shared_from_this();

		socket_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
			[this, self](boost::system::error_code ec, std::size_t bytes_transferred) {

				if (!ec) {
					CYNG_LOG_DEBUG(logger_, "[session] "
						<< vm_.get_tag() 
						<< " received "
						<< bytes_transferred 
						<< " bytes from [" 
						<< socket_.remote_endpoint() 
						<< "]");

#ifdef _DEBUG_IPT
					{
						std::stringstream ss;
						cyng::io::hex_dump<8> hd;
						hd(ss, buffer_.data(), buffer_.data() + bytes_transferred);
						CYNG_LOG_DEBUG(logger_, "[" << socket_.remote_endpoint() << "] " << bytes_transferred << " bytes:\n" << ss.str());
					}
#endif

					//
					//	let parse it
					//
					parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);

					//
					//	continue reading
					//
					do_read();
				}
				else {
					CYNG_LOG_WARNING(logger_, "[session] "
						<< vm_.get_tag() 
						<< " read: " << ec.message());
				}

		});
	}

	void ipt_session::do_write()
	{
		//if (is_stopped())	return;

		// Start an asynchronous operation to send a heartbeat message.
		boost::asio::async_write(socket_,
			boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
			cyng::expose_dispatcher(vm_).wrap(std::bind(&ipt_session::handle_write, this, std::placeholders::_1)));
	}

	void ipt_session::handle_write(const boost::system::error_code& ec)
	{
		//if (is_stopped())	return;

		if (!ec) {

			buffer_write_.pop_front();
			if (!buffer_write_.empty()) {
				do_write();
			}
			//else {

			//	// Wait 10 seconds before sending the next heartbeat.
			//	//heartbeat_timer_.expires_after(boost::asio::chrono::seconds(10));
			//	//heartbeat_timer_.async_wait(std::bind(&bus::do_write, this));
			//}
		}
		else {
			CYNG_LOG_ERROR(logger_, "[session] " 
				<< vm_.get_tag()
				<< " write: "
				<< ec.message());

			//reset();
		}
	}

	void ipt_session::ipt_cmd(ipt::header const& h, cyng::buffer_t&& body) {

		CYNG_LOG_TRACE(logger_, "[ipt] cmd " << ipt::command_name(h.command_));

		switch (ipt::to_code(h.command_)) {
		case ipt::code::CTRL_REQ_LOGIN_PUBLIC:
			if(cluster_bus_.is_connected()) {
				auto const [name, pwd] = ipt::ctrl_req_login_public(std::move(body));
				CYNG_LOG_INFO(logger_, "[ipt] public login: " << name << ':' << pwd);
				cluster_bus_.pty_login(name, pwd, vm_.get_tag(), "sml", socket_.remote_endpoint());
			}
			else {
				CYNG_LOG_WARNING(logger_, "[session] login rejected - MALFUNCTION");
				ipt_send(serializer_.res_login_public(ipt::ctrl_res_login_public_policy::MALFUNCTION, 0, ""));
			}
			break;
		case ipt::code::CTRL_REQ_LOGIN_SCRAMBLED:
			if (cluster_bus_.is_connected()) {
				auto const [name, pwd, sk] = ipt::ctrl_req_login_scrambled(std::move(body));
				CYNG_LOG_INFO(logger_, "[ipt] scrambled login: " << name << ':' << pwd);
				cluster_bus_.pty_login(name, pwd, vm_.get_tag(), "sml", socket_.remote_endpoint());
				parser_.set_sk(sk);
				serializer_.set_sk(sk);
			}
			else {
				CYNG_LOG_WARNING(logger_, "[session] login rejected - MALFUNCTION");
				ipt_send(serializer_.res_login_public(ipt::ctrl_res_login_public_policy::MALFUNCTION, 0, ""));
			}
			break;
		case ipt::code::APP_RES_SOFTWARE_VERSION:
			update_software_version(ipt::app_res_software_version(std::move(body)));
			break;
		case ipt::code::APP_RES_DEVICE_IDENTIFIER:
			update_device_identifier(ipt::app_res_device_identifier(std::move(body)));
			break;
		case ipt::code::CTRL_REQ_REGISTER_TARGET:
			if (cluster_bus_.is_connected()) {
				auto const [name, paket_size, window_size] = ipt::ctrl_req_register_target(std::move(body));
				register_target(name, paket_size, window_size, vm_.get_tag(), h.sequence_);
			}
			break;
		case ipt::code::CTRL_REQ_DEREGISTER_TARGET:
			if (cluster_bus_.is_connected()) {
				auto const name = ipt::ctrl_req_deregister_target(std::move(body));
				deregister_target(name, vm_.get_tag(), h.sequence_);
			}
			break;
		case ipt::code::TP_REQ_OPEN_PUSH_CHANNEL:
			if (cluster_bus_.is_connected()) {
				auto const [target, account, msisdn, version, id, timeout] = ipt::tp_req_open_push_channel(std::move(body));
				open_push_channel(target, account, msisdn, version, id, timeout, vm_.get_tag(), h.sequence_);
			}
			break;
		case ipt::code::TP_REQ_CLOSE_PUSH_CHANNEL:
			if (cluster_bus_.is_connected()) {
				auto const channel = ipt::ctrl_req_close_push_channel(std::move(body));
				close_push_channel(channel, vm_.get_tag(), h.sequence_);
			}
			break;
		case ipt::code::TP_REQ_PUSHDATA_TRANSFER:
			if (cluster_bus_.is_connected()) {
				auto const [channel, source, status, block, data] = ipt::tp_req_pushdata_transfer(std::move(body));
				pushdata_transfer(channel, source, status, block, data, vm_.get_tag(), h.sequence_);
			}
			break;
		default:
			CYNG_LOG_WARNING(logger_, "[ipt] cmd " << ipt::command_name(h.command_) << " dropped");
			ipt_send(serializer_.res_unknown_command(h.sequence_, h.command_));
			break;
		}

	}

	void ipt_session::register_target(std::string name
		, std::uint16_t paket_size
		, std::uint8_t window_size
		, boost::uuids::uuid tag
		, ipt::sequence_t seq) {

		CYNG_LOG_INFO(logger_, "[ipt] register: " << name);
		cluster_bus_.pty_reg_target(name, paket_size, window_size, dev_, tag, cyng::param_map_factory("seq", seq));
	}

	void ipt_session::deregister_target(std::string name
		, boost::uuids::uuid tag
		, ipt::sequence_t seq) {

		CYNG_LOG_INFO(logger_, "[ipt] deregister: " << name);
		cluster_bus_.pty_dereg_target(name, dev_, tag, cyng::param_map_factory("seq", seq));
	}

	void ipt_session::open_push_channel(std::string name
		, std::string account
		, std::string msisdn
		, std::string version
		, std::string id
		, std::uint16_t timeout
		, boost::uuids::uuid tag
		, ipt::sequence_t seq) {

		CYNG_LOG_INFO(logger_, "[ipt] open push channel: " << name);
		cluster_bus_.pty_open_channel(name, account, msisdn, version, id, std::chrono::seconds(timeout), dev_, tag, cyng::param_map_factory("seq", seq));

	}

	void ipt_session::close_push_channel(std::uint32_t channel
		, boost::uuids::uuid tag
		, ipt::sequence_t seq) {

		CYNG_LOG_INFO(logger_, "[ipt] close push channel: " << channel);
		cluster_bus_.pty_close_channel(channel, dev_, tag, cyng::param_map_factory("seq", seq));
	}

	void ipt_session::pushdata_transfer(std::uint32_t channel
		, std::uint32_t source
		, std::uint8_t status
		, std::uint8_t block
		, cyng::buffer_t data
		, boost::uuids::uuid tag
		, ipt::sequence_t seq) {

		CYNG_LOG_INFO(logger_, "[ipt] pushdata transfer: " << channel);
		cluster_bus_.pty_push_data(channel
			, source
			, std::move(data)
			, dev_
			, tag
			, cyng::param_map_factory("seq", seq)("status", status)("block", block));
	}


	void ipt_session::update_software_version(std::string str) {

		CYNG_LOG_INFO(logger_, "[ipt] software version: " << str);

		//
		//	update "device" table
		//
		cyng::param_map_t const data = cyng::param_map_factory()("vFirmware", str);
		cluster_bus_.req_db_update("device"
			, cyng::key_generator(dev_)
			, data);

	}
	void ipt_session::update_device_identifier(std::string str) {

		CYNG_LOG_INFO(logger_, "[ipt] device id: " << str);

		//
		//	update "device" table
		//
		cyng::param_map_t const data = cyng::param_map_factory()("id", str);
		cluster_bus_.req_db_update("device"
			, cyng::key_generator(dev_)
			, data);

	}

	void ipt_session::ipt_stream(cyng::buffer_t&& data) {
		CYNG_LOG_TRACE(logger_, "ipt stream " << data.size() << " byte");

	}

	void ipt_session::ipt_send(cyng::buffer_t&& data) {
		cyng::exec(vm_, [this, data]() {
			bool const b = buffer_write_.empty();
			buffer_write_.push_back(data);
			if (b)	do_write();
			});

	}

	void ipt_session::pty_res_login(bool success, boost::uuids::uuid dev) {
		if (success) {

			//
			//	update device tag
			//
			dev_ = dev;

			CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag()  << " login ok");
			ipt_send(serializer_.res_login_public(ipt::ctrl_res_login_public_policy::SUCCESS, 0, ""));

			query();

		}
		else {
			CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " login failed");
			ipt_send(serializer_.res_login_public(ipt::ctrl_res_login_public_policy::UNKNOWN_ACCOUNT, 0, ""));
		}
	}

	void ipt_session::pty_res_register(bool success, std::uint32_t channel, cyng::param_map_t token) {

		auto const reader = cyng::make_reader(token);
		auto const seq = cyng::value_cast<ipt::sequence_t>(reader["seq"].get(), 0);

		BOOST_ASSERT(seq != 0);
		if (success) {
			CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " register target ok: " << token);
		
			ipt_send(serializer_.res_register_push_target(seq, ipt::ctrl_res_register_target_policy::OK, channel));
		}
		else {
			CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " register target failed: " << token);
			ipt_send(serializer_.res_register_push_target(seq, ipt::ctrl_res_register_target_policy::REJECTED, channel));
		}
	}

	void ipt_session::pty_res_open_channel(bool success
		, std::uint32_t channel
		, std::uint32_t source
		, std::uint16_t packet_size
		, std::uint8_t window_size
		, std::uint8_t status
		, std::uint32_t count
		, cyng::param_map_t token) {

		auto const reader = cyng::make_reader(token);
		auto const seq = cyng::value_cast<ipt::sequence_t>(reader["seq"].get(), 0);

		BOOST_ASSERT(seq != 0);
		if (success) {
			CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " open push channel " << channel << ':' << source << " ok: " << token);

			ipt_send(serializer_.res_open_push_channel(seq
				, ipt::tp_res_open_push_channel_policy::SUCCESS
				, channel
				, source
				, packet_size
				, window_size
				, status
				, count));
		}
		else {
			CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " open push channel failed: " << token);
			ipt_send(serializer_.res_open_push_channel(seq, ipt::tp_res_open_push_channel_policy::UNREACHABLE, 0, 0, 0, 1, 0, 0));
		}
	}

	void ipt_session::pty_res_close_channel(bool success
		, std::uint32_t channel
		, std::size_t count
		, cyng::param_map_t token) {

		auto const reader = cyng::make_reader(token);
		auto const seq = cyng::value_cast<ipt::sequence_t>(reader["seq"].get(), 0);

		BOOST_ASSERT(seq != 0);
		if (success) {
			CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag() << " close push channel " << channel << " ok: " << token);
			ipt_send(serializer_.res_close_push_channel(seq, ipt::tp_res_close_push_channel_policy::SUCCESS, channel));
		}
		else {
			CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " close push channel " << channel << " failed: " << token);
			ipt_send(serializer_.res_close_push_channel(seq, ipt::tp_res_close_push_channel_policy::BROKEN, channel));
		}
	}

	void ipt_session::query() {

		if (ipt::test_bit(query_, ipt::query::PROTOCOL_VERSION)) {
			CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::PROTOCOL_VERSION)));
			ipt_send(serializer_.req_protocol_version());

		}
		if (ipt::test_bit(query_, ipt::query::FIRMWARE_VERSION)) {
			CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::FIRMWARE_VERSION)));
			ipt_send(serializer_.req_software_version());

		}
		if (ipt::test_bit(query_, ipt::query::DEVICE_IDENTIFIER)) {
			CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::DEVICE_IDENTIFIER)));
			ipt_send(serializer_.req_device_id());

		}
		if (ipt::test_bit(query_, ipt::query::NETWORK_STATUS)) {
			CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::NETWORK_STATUS)));
			ipt_send(serializer_.req_network_status());

		}
		if (ipt::test_bit(query_, ipt::query::IP_STATISTIC)) {
			CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::IP_STATISTIC)));
			ipt_send(serializer_.req_ip_statistics());
		}
		if (ipt::test_bit(query_, ipt::query::DEVICE_AUTHENTIFICATION)) {
			CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::DEVICE_AUTHENTIFICATION)));
			ipt_send(serializer_.req_device_auth());

		}
		if (ipt::test_bit(query_, ipt::query::DEVICE_TIME)) {
			CYNG_LOG_TRACE(logger_, "[pty] query: " << ipt::query_name(static_cast<std::uint32_t>(ipt::query::DEVICE_TIME)));
			ipt_send(serializer_.req_device_time());
		}
	}

	std::function<void(bool success, boost::uuids::uuid)>
	ipt_session::get_vm_func_pty_res_login(ipt_session* ptr) {
		return std::bind(&ipt_session::pty_res_login, ptr, std::placeholders::_1, std::placeholders::_2);
	}

	std::function<void(bool success, std::uint32_t, cyng::param_map_t)>
	ipt_session::get_vm_func_pty_res_register(ipt_session* ptr) {
		return std::bind(&ipt_session::pty_res_register, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	}

	std::function<void(bool success
		, std::uint32_t
		, std::uint32_t
		, std::uint16_t
		, std::uint8_t
		, std::uint8_t
		, std::uint32_t
		, cyng::param_map_t)>
	ipt_session::get_vm_func_pty_res_open_channel(ipt_session* ptr) {
		return std::bind(&ipt_session::pty_res_open_channel, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8);
	}

	std::function<void(bool success
		, std::uint32_t channel
		, std::size_t count
		, cyng::param_map_t)>
	ipt_session::get_vm_func_pty_res_close_channel(ipt_session* ptr) {
		return std::bind(&ipt_session::pty_res_close_channel, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	}

}


