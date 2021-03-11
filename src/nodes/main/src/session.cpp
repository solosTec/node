/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <session.h>
#include <tasks/server.h>

#include <cyng/log/record.h>
#include <cyng/io/serialize.h>
#include <cyng/vm/vm.h>
#include <cyng/vm/linearize.hpp>
#include <cyng/vm/generator.hpp>
#include <cyng/vm/mesh.h>

#include <iostream>

namespace smf {

	session::session(boost::asio::ip::tcp::socket socket, server* srv, cyng::logger logger)
	: socket_(std::move(socket))
		, logger_(logger)
		, buffer_()
		, buffer_write_()
		, vm_()
		, parser_([this](cyng::object&& obj) {
			CYNG_LOG_DEBUG(logger_, "parser: " << cyng::io::to_typed(obj));
			vm_.load(std::move(obj));
		})
	{
		vm_ = init_vm(srv);
		vm_.set_channel_name("cluster.req.login", 0);
		vm_.set_channel_name("pty.connect", 1);
	}

	session::~session()
	{
#ifdef _DEBUG_MAIN
		std::cout << "session(~)" << std::endl;
#endif
	}


	void session::stop()
	{
		//	https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload2.html
		CYNG_LOG_WARNING(logger_, "stop session");
		boost::system::error_code ec;
		socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
		socket_.close(ec);
	}

	void session::start()
	{
		do_read();
	}

	void session::do_read() {
		auto self = shared_from_this();

		socket_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
			[this, self](boost::system::error_code ec, std::size_t bytes_transferred) {

				if (!ec) {
					CYNG_LOG_DEBUG(logger_, "session [" << socket_.remote_endpoint() << "] received " << bytes_transferred << " bytes");

					//
					//	let parse it
					//
					parser_.read(buffer_.begin(), buffer_.begin() + bytes_transferred);

					//
					//	continue reading
					//
					do_read();
				}
				else {
					CYNG_LOG_WARNING(logger_, "[session] read: " << ec.message());
				}

		});
	}

	cyng::vm_proxy session::init_vm(server* srv) {

		std::function<void(std::string, std::string, cyng::pid, std::string, boost::uuids::uuid)> f1
			= std::bind(&session::cluster_login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);

		std::function<void(std::string)> f2 = std::bind(&server::pty_connect, srv, std::placeholders::_1);
		return srv->fabric_.create_proxy(f1, f2);
	}

	void session::do_write()
	{
		//if (is_stopped())	return;

		// Start an asynchronous operation to send a heartbeat message.
		boost::asio::async_write(socket_,
			boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
			std::bind(&session::handle_write, this, std::placeholders::_1));
	}

	void session::handle_write(const boost::system::error_code& ec)
	{
		//if (is_stopped())	return;

		if (!ec) {

			buffer_write_.pop_front();
			if (!buffer_write_.empty()) {
				do_write();
			}
			else {

				// Wait 10 seconds before sending the next heartbeat.
				//heartbeat_timer_.expires_after(boost::asio::chrono::seconds(10));
				//heartbeat_timer_.async_wait(std::bind(&bus::do_write, this));
			}
		}
		else {
			CYNG_LOG_ERROR(logger_, "[session] write: " << ec.message());

			//reset();
		}
	}

	void session::cluster_login(std::string name, std::string pwd, cyng::pid n, std::string node, boost::uuids::uuid tag) {
		CYNG_LOG_INFO(logger_, "session [" 
			<< socket_.remote_endpoint() 
			<< "] cluster login " 
			<< name << ":" 
			<< pwd 
			<< "@" << node 
			<< " #" << n.get_internal_value());

		//
		//	ToDo: check credentials
		//

		//
		//	send response
		//
		buffer_write_ = cyng::serialize_invoke("cluster.res.login", true);
		do_write();

	}

}


