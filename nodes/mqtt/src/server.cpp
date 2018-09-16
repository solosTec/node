/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "server.h"
//#include "connection.h"
//#include <smf/ipt/scramble_key_io.hpp>
//#include <smf/cluster/generator.h>
//#include <cyng/vm/generator.h>
//#include <cyng/dom/reader.h>
//#include <cyng/object_cast.hpp>
//#include <cyng/tuple_cast.hpp>
//#include <cyng/factory/set_factory.h>
//#include <boost/uuid/nil_generator.hpp>
//#include <boost/uuid/uuid_io.hpp>
//#include <boost/algorithm/string/predicate.hpp>

namespace node 
{
    namespace mqtt
	{
		server::server(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
            , bus::shared_type bus)
		: mux_(mux)
			, logger_(logger)
			, bus_(bus)
            , acceptor_(mux.get_io_service())
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
                    //  ToDo: call accept handler
                    //
//                    auto sp = std::make_shared<endpoint_t>(std::move(socket));
//                    if (cb_accept)  cb_accept(sp);

//					//
//					//	bus is synchronizing access to client_map_
//					//
//					bus_->vm_.async_run(cyng::generate_invoke("server.insert.connection", tag, conn));

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
}



