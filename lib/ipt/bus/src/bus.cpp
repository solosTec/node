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

			//
			//	set new scramble key after scrambled login request
			//
			vm_.async_run(cyng::register_function("ipt.set.sk", 1, [this](cyng::context& ctx) {
				const cyng::vector_t frame = ctx.get_frame();
				CYNG_LOG_TRACE(logger_, "set new scramble key "
					<< cyng::value_cast<scramble_key>(frame.at(0), scramble_key::null_scramble_key_));

				//	set new scramble key
				parser_.set_sk(cyng::value_cast<scramble_key>(frame.at(0), scramble_key::null_scramble_key_).key());
			}));

			//
			//	reset parser before reconnect
			//
			vm_.async_run(cyng::register_function("ipt.reset.parser", 1, [this](cyng::context& ctx) {
				const cyng::vector_t frame = ctx.get_frame();
				CYNG_LOG_TRACE(logger_, "reset parser "
					<< cyng::value_cast<scramble_key>(frame.at(0), scramble_key::null_scramble_key_));

				//	set new scramble key
				parser_.reset(cyng::value_cast<scramble_key>(frame.at(0), scramble_key::null_scramble_key_).key());
			}));
			

			//
			//	register ipt request handler
			//
			vm_.async_run(cyng::register_function("ipt.start", 0, [this](cyng::context& ctx) {
				this->start();
			}));

			//
			//	login response
			//
			vm_.async_run(cyng::register_function("ipt.res.login.public", 4, std::bind(&bus::ipt_res_login, this, std::placeholders::_1, false)));
			vm_.async_run(cyng::register_function("ipt.res.login.scrambled", 4, std::bind(&bus::ipt_res_login, this, std::placeholders::_1, true)));

			vm_.async_run(cyng::register_function("ipt.res.register.push.target", 4, std::bind(&bus::ipt_res_register_push_target, this, std::placeholders::_1)));
			vm_.async_run(cyng::register_function("ipt.req.transmit.data", 1, std::bind(&bus::ipt_req_transmit_data, this, std::placeholders::_1)));
			vm_.async_run(cyng::register_function("ipt.req.open.connection", 1, std::bind(&bus::ipt_req_open_connection, this, std::placeholders::_1)));
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
					CYNG_LOG_WARNING(logger_, "read <" << ec << ':' << ec.value() << ':' << ec.message() << '>');
					state_ = STATE_ERROR_;

					//
					//	slot [1] - go offline
					//
					mux_.send(task_, 1, cyng::tuple_t());

				}
			});

		}

		void bus::ipt_res_login(cyng::context& ctx, bool scrambled)
		{
			const cyng::vector_t frame = ctx.get_frame();

			//	[8d2c4721-7b0a-4ee4-ae25-63db3d5bc7bd,,30,]
			//CYNG_LOG_INFO(logger_, "ipt.res.login.public " << cyng::io::to_str(frame));

			//
			//	same for public and scrambled
			//
			const auto res = make_login_response(cyng::value_cast(frame.at(1), response_type(0)));
			const std::uint16_t watchdog = cyng::value_cast(frame.at(2), std::uint16_t(23));
			const std::string redirect = cyng::value_cast<std::string>(frame.at(3), "");

			if (scrambled)
			{
				ctx.run(cyng::generate_invoke("log.msg.trace", "ipt.res.login.scrambled", res.get_response_name()));
			}
			else
			{
				ctx.run(cyng::generate_invoke("log.msg.trace", "ipt.res.login.public", res.get_response_name()));
			}

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
		}

		void bus::ipt_res_register_push_target(cyng::context& ctx)
		{
			//	[79ba1da7-67b3-4c4f-9e1f-c217f5ea25a6,2,2,0]
			//
			//	* session tag
			//	* sequence
			//	* result code
			//	* channel
			const cyng::vector_t frame = ctx.get_frame();
			vm_.async_run(cyng::generate_invoke("log.msg.trace", "ipt.res.register.push.target", frame));

			const response_type res = cyng::value_cast<response_type>(frame.at(2), 0);
			ctrl_res_register_target_policy::get_response_name(res);
			vm_.async_run(cyng::generate_invoke("log.msg.info", "client.res.register.push.target", ctrl_res_register_target_policy::get_response_name(res)));
			//mux_.send(task_, 2, cyng::tuple_factory(channel));

		}

		void bus::ipt_req_transmit_data(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			vm_.async_run(cyng::generate_invoke("log.msg.trace", "ipt.req.transmit.data", frame));
		}

		void bus::ipt_req_open_connection(cyng::context& ctx)
		{
			//	[ipt.req.open.connection,[ec2451b6-67df-4942-a801-9e13ec7a448c,1,1024]]
			//
			//	* session tag
			//	* ipt sequence 
			//	* number
			//
			const cyng::vector_t frame = ctx.get_frame();
			vm_.async_run(cyng::generate_invoke("log.msg.trace", "ipt.req.open.connection", frame));

			switch (state_)
			{
			case STATE_AUTHORIZED_:
				// forward to session
				mux_.send(task_, 2, cyng::tuple_factory(frame.at(1), frame.at(2)));
				break;
			case STATE_CONNECTED_:
				//	busy
				vm_.async_run(cyng::generate_invoke("res.open.connection", frame.at(1), cyng::make_object<std::uint8_t>(ipt::tp_res_open_connection_policy::BUSY)));
				vm_.async_run(cyng::generate_invoke("stream.flush"));
				break;
			default:
				vm_.async_run(cyng::generate_invoke("res.open.connection", frame.at(1), cyng::make_object<std::uint8_t>(ipt::tp_res_open_connection_policy::DIALUP_FAILED)));
				vm_.async_run(cyng::generate_invoke("stream.flush"));
				break;
			}
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
