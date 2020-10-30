/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "session.h"
#include "../cache.h"
#include "../segw.h"

#include <smf/shared/db_cfg.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/status.h>

#include <cyng/vm/domain/log_domain.h>
#include <cyng/vm/domain/asio_domain.h>
#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace node
{
	namespace sml
	{
		session::session(boost::asio::ip::tcp::socket socket
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cache& cfg
			, storage& db
			, std::string const& account
			, std::string const& pwd
			, bool accept_all)
		: socket_(std::move(socket))
			, mux_(mux)
			, logger_(logger)
			, vm_(mux.get_io_service(), boost::uuids::random_generator()())
			, parser_([this](cyng::vector_t&& prg) {
				CYNG_LOG_TRACE(logger_, prg.size() << " SML instructions received (server)");
				CYNG_LOG_DEBUG(logger_, cyng::io::to_str(prg));
				vm_.async_run(std::move(prg));
			}, false, false, false)
			, router_(logger_
				, true	//	server mode
				, vm_
				, cfg
				, db
				, account
				, pwd
				, accept_all)
			, buffer_()
		{
			//
			//	register socket operations to session VM
			//
			cyng::register_socket(socket_, vm_);

			//
			//	register logger domain
			//
			cyng::register_logger(logger_, vm_);
			vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

			vm_.register_function("net.start.tsk.push", 8, [&](cyng::context& ctx) {

				//
				//	start <push> task
				//
				CYNG_LOG_WARNING(logger_, "cannot start <push> task "
					<< vm_.tag());

				auto const frame = ctx.get_frame();
				//CYNG_LOG_INFO(logger_, ctx.get_name()
				//	<< " - "
				//	<< cyng::io::to_str(frame));

				auto const tpl = cyng::tuple_cast<
					cyng::buffer_t,		//	[0] srv_id
					std::uint8_t,		//	[1] nr
					cyng::buffer_t,		//	[2] profile
					std::uint32_t,		//	[3] interval
					std::uint32_t,		//	[4] delay
					cyng::buffer_t,		//	[5] source
					std::string,		//	[6] target
					cyng::buffer_t		//	[7] service
				>(frame);

			});

			if (accept_all) {
				CYNG_LOG_WARNING(logger_, "new custom interface session "
					<< vm_.tag()
					<< " will accept all server IDs - this is a security risk");
			}
		}

		void session::start(cache& cfg)
		{
			//
			//	store IP address of custom interface
			//	ROOT_CUSTOM_PARAM - 81 02 00 07 10 FF
			//
			cfg.set_cfg(build_cfg_key({ sml::OBIS_ROOT_CUSTOM_PARAM }, "ep.remote"), socket_.remote_endpoint());
			cfg.set_cfg(build_cfg_key({ sml::OBIS_ROOT_CUSTOM_PARAM }, "ep.local"), socket_.local_endpoint());

			do_read();
		}

		void session::do_read()
		{
			auto self(shared_from_this());

			socket_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
				[this, self](boost::system::error_code ec, std::size_t bytes_transferred)
				{
					if (!ec) {
						parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);
						do_read();
					}
					else {
						CYNG_LOG_WARNING(logger_, "SML session closed <" << ec << ':' << ec.value() << ':' << ec.message() << '>');
					}
				});
		}
	}
}
