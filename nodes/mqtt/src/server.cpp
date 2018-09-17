/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "server.h"
#include <mqtt/str_qos.hpp>

namespace node 
{
	server::server(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, bus::shared_type bus)
	: mux_(mux)
		, logger_(logger)
		, bus_(bus)
		, acceptor_(mux.get_io_service())
		, socket_(new socket_t(mux_.get_io_service()))
	{
	}

	void server::run(std::string const& address, std::string const& service)
	{
		//CYNG_LOG_TRACE(logger_, "listen " 
		//	<< address
		//	<< ':'
		//	<< service);

		// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
		boost::asio::ip::tcp::resolver resolver(mux_.get_io_service());
#if (BOOST_VERSION >= 106600)
		boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(address, service).begin();
#else
		boost::asio::ip::tcp::resolver::query query(address, service);
		boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
#endif
		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor_.bind(endpoint);
		acceptor_.listen();

		CYNG_LOG_INFO(logger_, "listen "
			<< endpoint.address().to_string()
			<< ':'
			<< endpoint.port());

		do_accept();
	}

	void server::do_accept()
	{
#if (BOOST_VERSION >= 106600)
		acceptor_.async_accept(socket_->lowest_layer()
			, [this](boost::system::error_code ec) {
			// Check whether the server was stopped by a signal before this
			// completion handler had a chance to run.
			if (!acceptor_.is_open()) 
			{
				return;
			}

			if (!ec) 
			{
				CYNG_LOG_TRACE(logger_, "accept " << socket_->lowest_layer().remote_endpoint());

				//
				//  ToDo: call accept handler
				//
				auto sp = std::make_shared<endpoint_t>(std::move(socket_));

//				//
//				//	bus is synchronizing access to client_map_
//				//
//				bus_->vm_.async_run(cyng::generate_invoke("server.insert.connection", tag, conn));
				
				//
				//	set connection callbacks
				//
				set_callbacks(sp->shared_from_this());

				//
				//	new socket
				//
				socket_.reset(new socket_t(mux_.get_io_service()));
				
				//
				//
				//	continue accepting
				//
				do_accept();
			}

		});
#else
		acceptor_.async_accept(socket_, [=](boost::system::error_code const& ec) {
			// Check whether the server was stopped by a signal before this
			// completion handler had a chance to run.
			if (acceptor_.is_open() && !ec) {

				CYNG_LOG_TRACE(logger_, "accept " << socket_.remote_endpoint());
				//
				//	There is no connection manager or list of open connections. 
				//	Connections are managed by there own and are controlled
				//	by a maintenance task.
				//
				//std::make_shared<connection>(std::move(socket), mux_, logger_, db_)->start();
				do_accept();
			}

			else 
			{
				//on_error(ec);
				//	shutdown server
			}

		});
#endif

	}

	void server::set_callbacks(shared_ep sp)
	{
		sp->start_session(
			[sp] // keeping ep's lifetime as sp until session finished
			(boost::system::error_code const& ec) {
				std::cout << "session end: " << ec.message() << std::endl;
			}
		);
		
		// set connection (lower than MQTT) level handlers
		sp->set_close_handler([&](){
				std::cout << "closed." << std::endl;
				close_proc(sp->shared_from_this());
			});
		sp->set_error_handler([&](boost::system::error_code const& ec) {
				std::cout << "error: " << ec.message() << std::endl;
				close_proc(sp->shared_from_this());
			});
		
		// set MQTT level handlers
		sp->set_connect_handler([&](std::string const& client_id,
			 boost::optional<std::string> const& username,
			boost::optional<std::string> const& password,
			boost::optional<mqtt::will>,
			bool clean_session,
			std::uint16_t keep_alive) {
			
				std::cout << "client_id    : " << client_id << std::endl;
				std::cout << "username     : " << (username ? username.get() : "none") << std::endl;
				std::cout << "password     : " << (password ? password.get() : "none") << std::endl;
				std::cout << "clean_session: " << std::boolalpha << clean_session << std::endl;
				std::cout << "keep_alive   : " << keep_alive << std::endl;
				connections.insert(sp->shared_from_this());
				sp->connack(false, mqtt::connect_return_code::accepted);
				return true;
	}
		);
		sp->set_disconnect_handler([&]() {
			std::cout << "disconnect received." << std::endl;
			close_proc(sp->shared_from_this());
		});
		
		sp->set_puback_handler([&](std::uint16_t packet_id) {
			std::cout << "puback received. packet_id: " << packet_id << std::endl;
			return true;
		});
		
		sp->set_pubrec_handler([&](std::uint16_t packet_id) {
			std::cout << "pubrec received. packet_id: " << packet_id << std::endl;
			return true;
		});
		
		sp->set_pubrel_handler([&](std::uint16_t packet_id) {
			std::cout << "pubrel received. packet_id: " << packet_id << std::endl;
			return true;
		});
		
		sp->set_pubcomp_handler([&](std::uint16_t packet_id) {
			std::cout << "pubcomp received. packet_id: " << packet_id << std::endl;
			return true;
		});
		
		sp->set_publish_handler([&](std::uint8_t header,
			 boost::optional<std::uint16_t> packet_id,
			std::string topic_name,
			std::string contents) {
				std::uint8_t qos = mqtt::publish::get_qos(header);
				bool retain = mqtt::publish::is_retain(header);
				std::cout << "publish received."
				<< " dup: " << std::boolalpha << mqtt::publish::is_dup(header)
				<< " qos: " << mqtt::qos::to_str(qos)
				<< " retain: " << retain << std::endl;
				if (packet_id)
					std::cout << "packet_id: " << *packet_id << std::endl;
				std::cout << "topic_name: " << topic_name << std::endl;
				std::cout << "contents: " << contents << std::endl;
				auto const& idx = subs.get<tag_topic>();
				auto r = idx.equal_range(topic_name);
				for (; r.first != r.second; ++r.first) {
					r.first->con->publish(
						topic_name,
						contents,
						std::min(r.first->qos, qos),
										  retain
					);
				}
			return true;
		});
		
		sp->set_subscribe_handler([&](std::uint16_t packet_id,
			 std::vector<std::tuple<std::string, std::uint8_t>> entries) {
				std::cout << "subscribe received. packet_id: " << packet_id << std::endl;
				std::vector<std::uint8_t> res;
				res.reserve(entries.size());
				for (auto const& e : entries) {
					std::string const& topic = std::get<0>(e);
					std::uint8_t qos = std::get<1>(e);
					std::cout << "topic: " << topic  << " qos: " << static_cast<int>(qos) << std::endl;
					res.emplace_back(qos);
					subs.emplace(topic, sp->shared_from_this(), qos);
				}
				sp->suback(packet_id, res);
				return true;
		});
		
		sp->set_unsubscribe_handler([&](std::uint16_t packet_id,
			 std::vector<std::string> topics) {
				std::cout << "unsubscribe received. packet_id: " << packet_id << std::endl;
				for (auto const& topic : topics) {
					subs.erase(topic);
				}
				sp->unsuback(packet_id);
				return true;
		});
	}
	
	void server::close_proc(shared_ep con)
	{
		connections.erase(con);
		
		auto& idx = subs.get<tag_con>();
		auto r = idx.equal_range(con);
		idx.erase(r.first, r.second);		
	}
	
	void server::close()
	{
		//
		//	close acceptor
		//
		CYNG_LOG_INFO(logger_, "close acceptor");

		// The server is stopped by cancelling all outstanding asynchronous
		// operations. Once all operations have finished the io_context::run()
		// call will exit.
		acceptor_.close();

		//
		// close all clients
		//


	}
}



