/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 


#include "server.h"
#include "connection.h"
#include <smf/ipt/scramble_key_io.hpp>
#include <smf/cluster/generator.h>
#include <cyng/vm/generator.h>
#include <cyng/dom/reader.h>
#include <cyng/object_cast.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/predicate.hpp>

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
			//	connection management
			//
			bus_->vm_.run(cyng::register_function("push.connection", 1, std::bind(&server::push_connection, this, std::placeholders::_1)));
			bus_->vm_.run(cyng::register_function("server.insert.connection", 2, std::bind(&server::insert_connection, this, std::placeholders::_1)));
			bus_->vm_.run(cyng::register_function("server.close.connection", 1, std::bind(&server::close_connection, this, std::placeholders::_1)));

			//
			//	client responses
			//
			bus_->vm_.run(cyng::register_function("client.res.login", 7, std::bind(&server::client_res_login, this, std::placeholders::_1)));
			bus_->vm_.run(cyng::register_function("client.res.open.push.channel", 7, std::bind(&server::client_res_open_push_channel, this, std::placeholders::_1)));
			bus_->vm_.run(cyng::register_function("client.res.register.push.target", 1, std::bind(&server::client_res_register_push_target, this, std::placeholders::_1)));
			bus_->vm_.run(cyng::register_function("client.res.open.connection", 6, std::bind(&server::client_res_open_connection, this, std::placeholders::_1)));
			bus_->vm_.run(cyng::register_function("client.req.open.connection.forward", 0, std::bind(&server::client_req_open_connection_forward, this, std::placeholders::_1)));
			bus_->vm_.run(cyng::register_function("client.res.open.connection.forward", 0, std::bind(&server::client_res_open_connection_forward, this, std::placeholders::_1)));
			bus_->vm_.run(cyng::register_function("client.req.transmit.data.forward", 0, std::bind(&server::client_req_transmit_data_forward, this, std::placeholders::_1)));
			bus_->vm_.run(cyng::register_function("client.res.transfer.pushdata", 7, std::bind(&server::client_res_transfer_pushdata, this, std::placeholders::_1)));
			bus_->vm_.run(cyng::register_function("client.req.transfer.pushdata.forward", 7, std::bind(&server::client_req_transfer_pushdata_forward, this, std::placeholders::_1)));
			bus_->vm_.run(cyng::register_function("client.res.close.push.channel", 6, std::bind(&server::client_res_close_push_channel, this, std::placeholders::_1)));

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
					//	create new connection/session
					//
					auto tag = rnd_();
					auto conn = make_connection(std::move(socket)
						, mux_
						, logger_
						, bus_
						, tag
						, sk_
						, watchdog_
						, timeout_);

					//
					//	bus is synchronizing access to client_map_
					//
					bus_->vm_.async_run(cyng::generate_invoke("server.insert.connection", tag, conn));

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
			// The server is stopped by cancelling all outstanding asynchronous
			// operations. Once all operations have finished the io_context::run()
			// call will exit.
			acceptor_.close();
		}

		void server::insert_connection(cyng::context& ctx)
		{
			BOOST_ASSERT(bus_->vm_.tag() == ctx.tag());

			//	[a95f46e9-eccd-4b47-b02c-17d5172218af,<!260:ipt::connection>]
			const cyng::vector_t frame = ctx.get_frame();

			auto tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
			auto r = client_map_.emplace(tag, frame.at(1));
			if (r.second)
			{
				ctx.attach(cyng::generate_invoke("log.msg.trace", "server.insert.connection", frame));
				const_cast<connection*>(cyng::object_cast<connection>((*r.first).second))->start();
			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.error", "server.insert.connection - failed", frame));
			}
		}

		//	"server.close.connection"
		void server::close_connection(cyng::context& ctx)
		{
			BOOST_ASSERT(bus_->vm_.tag() == ctx.tag());

			//	[6ac8cc52-18ed-43f5-a86c-ef948f0d960f,system:10054]
			const cyng::vector_t frame = ctx.get_frame();
			auto tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
			if (client_map_.erase(tag) != 0)
			{
				ctx.attach(client_req_close(tag, 0));
				ctx.attach(cyng::generate_invoke("log.msg.info", "server.close.connection", frame));
			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.error", "server.close.connection - failed", frame));
			}
		}

		void server::push_connection(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			auto tag = cyng::value_cast(frame.at(0), boost::uuids::nil_uuid());
			auto pos = client_map_.find(tag);
			if (pos != client_map_.end())
			{
				CYNG_LOG_TRACE(logger_, "push connection "
					<< tag
					<< " on stack");
				ctx.push(pos->second);
			}
			else
			{
				CYNG_LOG_FATAL(logger_, "push connection "
					<< tag
					<< " not found");
				ctx.push(cyng::make_object());
			}
		}

		void server::client_res_login(cyng::context& ctx)
		{
			//	[068544fb-9513-4cbe-9007-c9dd9892aff6,d03ff1a5-838a-4d71-91a1-fc8880b157a6,17,true,OK,%(("tp-layer":ipt))]
			//
			//	* client tag
			//	* peer
			//	* sequence
			//	* response (bool)
			//	* name
			//	* msg (optional)
			//	* query
			//	* bag
			//	
			propagate("client.res.login", ctx.get_frame());
		}

		void server::propagate(std::string fun, cyng::vector_t&& msg)
		{
			const boost::uuids::uuid tag = cyng::value_cast(msg.at(0), boost::uuids::nil_uuid());
			auto pos = client_map_.find(tag);
			if (pos != client_map_.end())
			{
				cyng::vector_t prg;
				prg
					<< cyng::code::ESBA
					<< cyng::unwinder(std::move(msg))
					<< cyng::invoke(fun)
					<< cyng::code::REBA
					;
				cyng::object_cast<connection>((*pos).second)->session_.vm_.async_run(std::move(prg));
			}
			else
			{
				CYNG_LOG_ERROR(logger_, "session "
					<< tag
					<< " not found");

#ifdef _DEBUG
				for (auto client : client_map_)
				{
					CYNG_LOG_TRACE(logger_, client.first);
				}
#endif
			}
		}

		void server::client_res_open_push_channel(cyng::context& ctx)
		{
			propagate("client.res.open.push.channel", ctx.get_frame());
		}

		void server::client_res_register_push_target(cyng::context& ctx)
		{
			propagate("client.res.register.push.target", ctx.get_frame());
		}

		void server::client_res_open_connection(cyng::context& ctx)
		{
			propagate("client.res.open.connection", ctx.get_frame());
		}

		void server::client_req_open_connection_forward(cyng::context& ctx)
		{
			propagate("client.req.open.connection.forward", ctx.get_frame());
		}

		void server::client_res_open_connection_forward(cyng::context& ctx)
		{
			propagate("client.res.open.connection.forward", ctx.get_frame());
		}

		void server::client_req_transmit_data_forward(cyng::context& ctx)
		{
			propagate("client.req.transmit.data.forward", ctx.get_frame());
		}

		void server::client_res_transfer_pushdata(cyng::context& ctx)
		{
			propagate("client.res.transfer.pushdata", ctx.get_frame());
		}

		void server::client_req_transfer_pushdata_forward(cyng::context& ctx)
		{
			propagate("client.req.transfer.pushdata.forward", ctx.get_frame());
		}

		void server::client_res_close_push_channel(cyng::context& ctx)
		{
			propagate("client.res.close.push.channel", ctx.get_frame());
		}

	}
}



