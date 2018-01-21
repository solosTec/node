/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "server.h"
#include "connection.h"
#include <smf/ipt/scramble_key_io.hpp>
#include <cyng/vm/generator.h>
#include <boost/uuid/nil_generator.hpp>

namespace node 
{
	namespace ipt
	{
		server::server(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, scramble_key const& sk
			, uint16_t watchdog
			, int timeout)
		: mux_(mux)
			, logger_(logger)
			, bus_(bus)
			, sk_(sk)
			, watchdog_(watchdog)
			, timeout_(timeout)
			, acceptor_(mux.get_io_service())
#if (BOOST_VERSION < 106600)
			, socket_(io_ctx_)
#endif
			, rnd_()
			, client_map_()
		{
			//
			//	client responses
			//
			bus_->vm_.async_run(cyng::register_function("client.res.login", 1, std::bind(&server::client_res_login, this, std::placeholders::_1)));

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
			acceptor_.async_accept(
				[this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
				// Check whether the server was stopped by a signal before this
				// completion handler had a chance to run.
				if (!acceptor_.is_open()) 
				{
					return;
				}

				if (!ec) 
				{
					CYNG_LOG_TRACE(logger_, "accept " << socket.remote_endpoint());

					//
					//	There is no connection manager or list of open connections. 
					//	Connections are managed by there own and are controlled
					//	by a maintenance task.
					//
					//	ToDo: implement as bus method for better locking
					//
					auto tag = rnd_();
					auto r = client_map_.emplace(tag, std::make_shared<connection>(std::move(socket)
						, mux_
						, logger_
						, bus_
						, tag
						, sk_
						, watchdog_
						, timeout_));
					if (r.second)
					{
						(*r.first).second->start();
					}

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
					std::make_shared<connection>(std::move(socket), mux_, logger_, db_)->start();
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

		void server::close()
		{
			// The server is stopped by cancelling all outstanding asynchronous
			// operations. Once all operations have finished the io_context::run()
			// call will exit.
			acceptor_.close();
		}

		void server::client_res_login(cyng::context& ctx)
		{
			//	[28ea3bb4-34d6-4e14-8f0f-b2257810d149,1b776e3d-b454-41b8-a760-dfdebd7dec40,1,true,OK,("tp-layer":ipt)]
			//
			//	* client tag
			//	* peer
			//	* sequence
			//	* response (bool)
			//	* msg (optional)
			//	* bag
			//	
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "client.res.login " << cyng::io::to_str(frame));

			const boost::uuids::uuid tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());

			auto pos = client_map_.find(tag);
			if (pos != client_map_.end())
			{
				//const boost::uuids::uuid tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
				const std::uint64_t seq = cyng::value_cast<std::uint64_t>(frame.at(2), 0);
				//BOOST_ASSERT_MSG(seq == 1, "wrong sequence");

				const bool success = cyng::value_cast(frame.at(3), false);
				const std::string msg = cyng::value_cast<std::string>(frame.at(4), "");

				(*pos).second->session_.client_res_login(seq, success, msg);

			}
		}

	}
}



