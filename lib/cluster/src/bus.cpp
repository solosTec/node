/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/cluster/bus.h>
#include <smf/cluster/generator.h>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/vm/domain/asio_domain.h>
#include <cyng/value_cast.hpp>
#include <cyng/tuple_cast.hpp>
#include <cyng/io/io_chrono.hpp>
#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
#endif
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node
{
	bus::bus(cyng::async::mux& mux, cyng::logging::log_ptr logger, boost::uuids::uuid tag, std::size_t tsk)
		: vm_(mux.get_io_service(), tag)
		, socket_(mux.get_io_service())
		, buffer_()
		, mux_(mux)
		, logger_(logger)
		, task_(tsk)
		, parser_([this](cyng::vector_t&& prg) {
			vm_.async_run(std::move(prg));
		})
		, serializer_(socket_, vm_)
		, state_(state::INITIAL_)
		, remote_tag_(boost::uuids::nil_uuid())
		, remote_version_(0, 0)
		, seq_(0)
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
		//	increase sequence and set as result value
		//
		vm_.register_function("bus.seq.next", 0, [this](cyng::context& ctx) {
			++this->seq_;
			ctx.push(cyng::make_object(this->seq_));
		});

		//
		//	push last bus sequence number on stack
		//
		vm_.register_function("bus.seq.push", 0, [this](cyng::context& ctx) {
			ctx.push(cyng::make_object(this->seq_));
		});


		//
		//	register bus request handler
		//
		vm_.register_function("bus.start", 0, [this](cyng::context& ctx) {
			this->start();
		});

		vm_.register_function("bus.res.login", 5, [this](cyng::context& ctx) {
			auto const frame = ctx.get_frame();

			CYNG_LOG_TRACE(logger_, "login response " << cyng::io::to_str(frame));

			if (cyng::value_cast(frame.at(0), false))
			{
				state_ = state::AUTHORIZED_;
				remote_tag_ = cyng::value_cast(frame.at(1), boost::uuids::nil_uuid());
				remote_version_ = cyng::value_cast(frame.at(2), remote_version_);
				ctx.run(cyng::generate_invoke("log.msg.info", "successful cluster login ", remote_tag_, " v", remote_version_));

				auto lag = std::chrono::system_clock::now() - cyng::value_cast(frame.at(3), std::chrono::system_clock::now());
				CYNG_LOG_TRACE(logger_, "cluster login lag: " << cyng::to_str(lag));

				//
				//	slot [0]
				//
				CYNG_LOG_TRACE(logger_, "send cluster login response to task #" << task_);
				mux_.post(task_, 0, cyng::tuple_factory(remote_version_));

			}
			else
			{
				state_ = state::ERROR_;
				ctx.run(cyng::generate_invoke("log.msg.warning", "cluster login failed"));

				//
				//	slot [1] - go offline
				//
				mux_.post(task_, 1, cyng::tuple_t{});
			}

		});

		vm_.register_function("bus.req.watchdog", 3, [this](cyng::context& ctx) {
			const cyng::vector_t frame = ctx.get_frame();
			//	[42dd5709-be73-45ae-98b3-35df10765015,5,2018-03-19 17:55:56.48749690]
			//
			//	* remote session
			//	* remote time stamp
			//	* sequence
			//CYNG_LOG_INFO(logger_, "bus.req.watchdog " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] session tag
				std::uint64_t,			//	[1] sequence
				std::chrono::system_clock::time_point	//	[2] timestamp
			>(frame);

			//
			//	send watchdog response
			//
			ctx.queue(bus_res_watchdog(std::get<1>(tpl), std::get<2>(tpl)));
		});

		vm_.async_run(cyng::generate_invoke("log.msg.debug", cyng::invoke("lib.size"), " callbacks registered"));

	}

	void bus::start()
	{
		//CYNG_LOG_TRACE(logger_, "start cluster bus");
        state_ = state::INITIAL_;
		do_read();
	}

    void bus::stop()
    {
        CYNG_LOG_TRACE(logger_, "stop cluster bus");

        //
        //  update state
        //
        state_ = state::SHUTDOWN_;

		//
		//  close socket
		//
		boost::system::error_code ec;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket_.close(ec);

        //
        //  no more callbacks
        //
		auto self(this->shared_from_this());
		vm_.access([self](cyng::vm& vm) {
			vm.run(cyng::generate_invoke("log.msg.info", "cluster bus shutdown"));
			vm.run(cyng::vector_t{ cyng::make_object(cyng::code::HALT) });
		});
    }

    bool bus::is_online() const
	{
		return state_ == state::AUTHORIZED_;
	}

	void bus::do_read()
	{
        //
        //  do nothing during shutdown
        //
        if (state_ == state::SHUTDOWN_)  return;

		auto self(this->shared_from_this());
		socket_.async_read_some(boost::asio::buffer(buffer_),
			[this, self](boost::system::error_code ec, std::size_t bytes_transferred)
		{
			if (!ec)
			{
				//CYNG_LOG_TRACE(logger_, "cluster bus " << vm_.tag() << " received " << bytes_transferred << " bytes");
#ifdef SMF_IO_DEBUG
				cyng::io::hex_dump hd;
				std::stringstream ss;
				hd(ss, buffer_.data(), buffer_.data() + bytes_transferred);
				CYNG_LOG_TRACE(logger_, "cluster input dump \n" << ss.str());
#endif
				parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);
				do_read();
			}
			else if (ec != boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, "cluster read <" << ec << ':' << ec.message() << '>');
				state_ = state::ERROR_;
				remote_tag_ = boost::uuids::nil_uuid();
				remote_version_ = cyng::version(0, 0);
				//lag_ = std::chrono::microseconds::max();

				//
				//	slot [1] - go offline
				//
				mux_.post(task_, 1, cyng::tuple_t{});

			}
			else
			{
				//
				//	The connection was closed intentionally.
				//
			}
		});

	}


	bus::shared_type bus_factory(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, std::size_t tsk)
	{
		return std::make_shared<bus>(mux, logger, tag, tsk);
	}

}
