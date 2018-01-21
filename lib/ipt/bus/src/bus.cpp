/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include <smf/ipt/bus.h>
#include <smf/ipt/scramble_key_io.hpp>
#include <smf/ipt/response.hpp>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/vm/domain/asio_domain.h>
#include <cyng/value_cast.hpp>
#include <cyng/io/serializer.h>
#include <boost/uuid/nil_generator.hpp>
#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

namespace node
{
	namespace ipt
	{
		bus::bus(cyng::async::mux& mux, cyng::logging::log_ptr logger, boost::uuids::uuid tag, scramble_key const& sk, std::size_t tsk)
		: vm_(mux.get_io_service(), tag)
			, socket_(mux.get_io_service())
			, buffer_()
			, mux_(mux)
			, logger_(logger)
			, task_(tsk)
			, parser_([this](cyng::vector_t&& prg) {
				CYNG_LOG_INFO(logger_, prg.size() << " instructions received");
				vm_.async_run(std::move(prg));
			}, sk)
			, serializer_(socket_, vm_, sk)
			, state_(STATE_INITIAL_)
		{
			//
			//	register logger domain
			//
			cyng::register_logger(logger_, vm_);
			vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

			//
			//	register socket domain
			//
			cyng::register_socket(socket_, vm_);
			vm_.async_run(cyng::generate_invoke("log.msg.info", "ip:tcp:socket domain is running"));

			vm_.async_run(cyng::register_function("ipt.set.sk", 1, [this](cyng::context& ctx) {
				const cyng::vector_t frame = ctx.get_frame();
				CYNG_LOG_TRACE(logger_, "set new scramble key "
					<< cyng::value_cast<scramble_key>(frame.at(0), scramble_key::null_scramble_key_));

				//	set new scramble key
				parser_.set_sk(cyng::value_cast<scramble_key>(frame.at(0), scramble_key::null_scramble_key_).key());
				//scrambler_ = cyng::value_cast(frame.at(0), def_key_).key();
			}));

			//
			//	register ipt request handler
			//
			vm_.async_run(cyng::register_function("ipt.start", 0, [this](cyng::context& ctx) {
				this->start();
			}));

			vm_.async_run(cyng::register_function("ipt.res.login.public", 4, [this](cyng::context& ctx) {
				const cyng::vector_t frame = ctx.get_frame();

				//	[8d2c4721-7b0a-4ee4-ae25-63db3d5bc7bd,,30,]
				//CYNG_LOG_INFO(logger_, "ipt.res.login.public " << cyng::io::to_str(frame));
				const auto res = make_login_response(cyng::value_cast(frame.at(1), response_type(0)));
				const std::uint16_t watchdog = cyng::value_cast(frame.at(2), std::uint16_t(23));
				const std::string redirect = cyng::value_cast<std::string>(frame.at(3), "");

				CYNG_LOG_INFO(logger_, "ipt.res.login.public " << res.get_response_name());

				if (res.is_success())
				{
					ctx.run(cyng::generate_invoke("log.msg.info", "successful authorized"));
					state_ = STATE_AUTHORIZED_;

					//
					//	slot [0]
					//
					mux_.send(task_, 0, cyng::tuple_factory(watchdog, redirect));

				}
				else
				{
					state_ = STATE_ERROR_;

					//
					//	slot [1]
					//
					mux_.send(task_, 1, cyng::tuple_t());
				}

			}));
			vm_.async_run(cyng::register_function("ipt.res.login.scrambled", 4, [this](cyng::context& ctx) {
				const cyng::vector_t frame = ctx.get_frame();

				//	[8d2c4721-7b0a-4ee4-ae25-63db3d5bc7bd,,30,]
				//CYNG_LOG_INFO(logger_, "ipt.res.login.scrambled " << cyng::io::to_str(frame));

				const auto res = make_login_response(cyng::value_cast(frame.at(1), response_type(0)));
				const std::uint16_t watchdog = cyng::value_cast(frame.at(2), std::uint16_t(23));
				const std::string redirect = cyng::value_cast<std::string>(frame.at(3), "");

				CYNG_LOG_INFO(logger_, "ipt.res.login.scrambled " << res.get_response_name());

				if (res.is_success())
				{
					ctx.run(cyng::generate_invoke("log.msg.info", "successful authorized"));
					state_ = STATE_AUTHORIZED_;

					//
					//	slot [0]
					//
					mux_.send(task_, 0, cyng::tuple_factory(watchdog, redirect));

				}
				else
				{
					state_ = STATE_ERROR_;

					//
					//	slot [1]
					//
					mux_.send(task_, 1, cyng::tuple_t());
				}

			}));

		}

		void bus::start()
		{
			CYNG_LOG_TRACE(logger_, "start ipt network");
			do_read();
		}

		bool bus::is_online() const
		{
			return state_ == STATE_AUTHORIZED_;
		}

		void bus::do_read()
		{
			auto self(shared_from_this());
			socket_.async_read_some(boost::asio::buffer(buffer_),
				[this, self](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					//CYNG_LOG_TRACE(logger_, bytes_transferred << " bytes read");
					vm_.async_run(cyng::generate_invoke("log.msg.trace", "ipt client received", bytes_transferred, "bytes"));
					const auto buffer = parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);

#ifdef SMF_IO_DEBUG
					cyng::io::hex_dump hd;
					std::stringstream ss;
					hd(ss, buffer.begin(), buffer.end());
					CYNG_LOG_TRACE(logger_, "ipt input dump \n" << ss.str());
#endif

					do_read();
				}
				else
				{
					CYNG_LOG_WARNING(logger_, "read <" << ec << ':' << ec.message() << '>');
					state_ = STATE_ERROR_;
				}
			});

		}


		bus::shared_type bus_factory(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, boost::uuids::uuid tag
			, scramble_key const& sk
			, std::size_t tsk)
		{
			return std::make_shared<bus>(mux, logger, tag, sk, tsk);
		}
	}
}
