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

#include <cyng/log/record.h>
#include <cyng/vm/mesh.h>
#include <cyng/vm/vm.h>

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
		, cyng::logger logger)
	: socket_(std::move(socket))
		, logger_(logger)
		, cluster_bus_(cluster_bus)
		, buffer_()
		, buffer_write_()
		, parser_(sk
			, std::bind(&ipt_session::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&ipt_session::ipt_stream, this, std::placeholders::_1)
		)
		, serializer_(sk)
		, vm_()
	{
		vm_ = fabric.create_proxy(cluster_bus_.get_tag()
			, get_vm_func_pty_res_login(this));

		vm_.set_channel_name("pty.res.login", 0);

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
			std::bind(&ipt_session::handle_write, this, std::placeholders::_1));
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
				buffer_write_.push_back(serializer_.res_login_public(ipt::ctrl_res_login_public_policy::MALFUNCTION, 0, ""));
				do_write();
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
				buffer_write_.push_back(serializer_.res_login_public(ipt::ctrl_res_login_public_policy::MALFUNCTION, 0, ""));
				do_write();
			}
			break;
		default:
			break;
		}

	}
	void ipt_session::ipt_stream(cyng::buffer_t&& data) {
		CYNG_LOG_TRACE(logger_, "ipt stream " << data.size() << " byte");

	}

	void ipt_session::pty_res_login(bool success) {
		if (success) {
			CYNG_LOG_INFO(logger_, "[pty] " << vm_.get_tag()  << " login ok");
		}
		else {
			CYNG_LOG_WARNING(logger_, "[pty] " << vm_.get_tag() << " login failed");
		}
	}

	std::function<void(bool success)>
	ipt_session::get_vm_func_pty_res_login(ipt_session* ptr) {
		return std::bind(&ipt_session::pty_res_login, ptr, std::placeholders::_1);
	}

}


