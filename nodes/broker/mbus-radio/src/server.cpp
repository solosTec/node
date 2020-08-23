/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "server.h"
#include "session.h"
#include <smf/cluster/generator.h>

#include <cyng/vm/controller.h>
#include <cyng/io/serializer.h>

namespace node
{
	server::server(cyng::io_service_t& ios
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, boost::asio::ip::tcp::endpoint ep)
	: acceptor_(ios, ep)
		, logger_(logger)
		, vm_(vm)
		, session_counter_{ 0 }
		, uuid_gen_{}
	{
		CYNG_LOG_INFO(logger_, "configured local endpoint " << ep);

		vm_.register_function("client.res.login", 1, [&](cyng::context& ctx) {

			auto frame = ctx.get_frame();

			//	[67dd63f3-a055-41de-bdee-475f71621a21,26c841a3-a1c4-4ac2-92f6-8bdd2cbe80f4,1,false,name,unknown device,00000000,
			//	%(("data-layer":wM-Bus),("local-ep":127.0.0.1:63953),("remote-ep":127.0.0.1:12001),("security":public),("time":2020-08-23 18:02:59.18770200),("tp-layer":raw))]

			//	[67dd63f3-a055-41de-bdee-475f71621a21,[26c841a3-a1c4-4ac2-92f6-8bdd2cbe80f4,1,false,name,unknown device,00000000,
			//	%(("data-layer":wM-Bus),("local-ep":127.0.0.1:63953),("remote-ep":127.0.0.1:12001),("security":public),("time":2020-08-23 18:02:59.18770200),("tp-layer":raw))]]
			CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			//
			//	get tag from adressed child VM
			//
			auto pos = frame.begin();
			auto const tag = cyng::value_cast(*pos, uuid_gen_());

			//
			//	remove first element
			//
			frame.erase(pos);

			ctx.forward(tag, cyng::generate_invoke(ctx.get_name(), tag, frame));

		});
		
	}

	void server::run()
	{
		do_accept();
	}

	void server::close()
	{
		acceptor_.cancel();
		acceptor_.close();
	}
	 
	void server::do_accept()
	{
		acceptor_.async_accept(
			[this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
			{
				if (!ec)
				{
					CYNG_LOG_TRACE(logger_, "start session at " << socket.remote_endpoint());

					auto const tag = uuid_gen_();
					auto sp = std::shared_ptr<session>(new session(std::move(socket), logger_, vm_, vm_.emplace(tag)), [this, tag](session* s) {

						//
						//	update session counter
						//
						--session_counter_;
						CYNG_LOG_TRACE(logger_, "session count " << session_counter_);
						if (!vm_.is_halted()) {
							vm_.async_run(bus_update_client_count(session_counter_));
						}

						//
						//	remove from VM controller
						//
						vm_.async_run(cyng::generate_invoke("vm.remove", tag));

						//
						//	remove session
						//
						delete s;
					});

					if (sp) {

						//
						//	start session
						//
						sp->start();

						//
						//	update session counter
						//
						++session_counter_;
						vm_.async_run(bus_update_client_count(session_counter_));

					}
				}

				do_accept();
			});
	}

}
