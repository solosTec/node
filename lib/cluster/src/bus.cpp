/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include <smf/cluster/bus.h>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/vm/domain/asio_domain.h>
#include <cyng/value_cast.hpp>
#include <cyng/io/io_chrono.hpp>
#include <boost/uuid/nil_generator.hpp>

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
			CYNG_LOG_INFO(logger_, prg.size() << " instructions received");
			vm_.async_run(std::move(prg));
		})
		, serializer_(socket_, vm_)
		, state_(STATE_INITIAL_)
		, remote_tag_(boost::uuids::nil_uuid())
		, remote_version_(0, 0)
		, lag_(std::chrono::microseconds::max())
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
		//	register bus request handler
		//
		vm_.async_run(cyng::register_function("bus.start", 0, [this](cyng::context& ctx) {
			this->start();
		}));

		vm_.async_run(cyng::register_function("bus.res.login", 5, [this](cyng::context& ctx) {
			const cyng::vector_t frame = ctx.get_frame();

			//	[true,435a75e4-b97d-4152-a9b7-cdc2e21e8599,0.2,2018-01-11 16:54:48.89934220,2018-01-11 16:54:48.91710660]
			CYNG_LOG_INFO(logger_, "login response " << cyng::io::to_str(frame));			

			if (cyng::value_cast(frame.at(0), false))
			{
				ctx.run(cyng::generate_invoke("log.msg.info", "successful authorized"));
				state_ = STATE_AUTHORIZED_;
				remote_tag_ = cyng::value_cast(frame.at(1), boost::uuids::nil_uuid());
				remote_version_ = cyng::value_cast(frame.at(2), remote_version_);
				lag_ = std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::system_clock::now() - cyng::value_cast(frame.at(3), std::chrono::system_clock::now()));
				CYNG_LOG_TRACE(logger_, "lag " << lag_.count() << " microsec");

				//
				//	slot [0]
				//
				mux_.send(task_, 0, cyng::tuple_factory(remote_version_));

			}
			else
			{
				state_ = STATE_ERROR_;

				//
				//	slot [1] - go offline
				//
				mux_.send(task_, 1, cyng::tuple_t());
			}

		}));

		//
		//	increase sequence and set as resulz value
		//
		vm_.async_run(cyng::register_function("bus.seq.next", 0, [this](cyng::context& ctx) {
			++this->seq_;
			ctx.push(cyng::make_object(this->seq_));
			//ctx.set_return_value(cyng::make_object(this->seq_), 0);
		}));

		
	}

	void bus::start()
	{
		CYNG_LOG_TRACE(logger_, "start cluster bus");
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
				CYNG_LOG_TRACE(logger_, bytes_transferred << " bytes read");
				parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);
				do_read();
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "read <" << ec << ':' << ec.message() << '>');
				state_ = STATE_ERROR_;
				remote_tag_ = boost::uuids::nil_uuid();
				remote_version_ = cyng::version(0, 0);
				lag_ = std::chrono::microseconds::max();

				//
				//	slot [1] - go offline
				//
				mux_.send(task_, 1, cyng::tuple_t());

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
