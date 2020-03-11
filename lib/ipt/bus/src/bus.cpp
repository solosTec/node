﻿/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/ipt/bus.h>
#include <smf/ipt/scramble_key_io.hpp>
#include <smf/ipt/response.hpp>
#include <smf/ipt/generator.h>

#include <NODE_project_info.h>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/vm/domain/asio_domain.h>
#include <cyng/value_cast.hpp>
#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/async/task/task_builder.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
#include <fstream>
#endif
#include "tasks/open_connection.h"
#include "tasks/close_connection.h"
#include "tasks/register_target.h"
#include "tasks/watchdog.h"
#include "tasks/open_channel.h"
#include "tasks/transfer_data.h"

namespace node
{
	namespace ipt
	{
		bus::bus(cyng::logging::log_ptr logger
			, cyng::async::mux& mux
			, boost::uuids::uuid tag
			, std::string const& model
			, std::size_t retries)
		: logger_(logger)
			, vm_(mux.get_io_service(), tag)
			, mux_(mux)
			, socket_(mux.get_io_service())
			, buffer_()
			, parser_([this](cyng::vector_t&& prg) {
				CYNG_LOG_DEBUG(logger_, prg.size() << " IPT instructions received");
#ifdef SMF_IO_DEBUG
				CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
#endif
				vm_.async_run(std::move(prg));
			}, scramble_key::default_scramble_key_)
			, serializer_(socket_, vm_, scramble_key::default_scramble_key_)
			, model_(model)
			, retries_((retries == 0) ? 1 : retries)
			, watchdog_(0u)
			, state_(STATE_INITIAL_)
			, task_db_()
			, channel_db_()
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
			vm_.register_function("ipt.set.sk", 1, [this](cyng::context& ctx) {
				const cyng::vector_t frame = ctx.get_frame();
				CYNG_LOG_TRACE(logger_, "set new scramble key "
					<< cyng::value_cast<scramble_key>(frame.at(0), scramble_key::null_scramble_key_));

				//	set new scramble key
				parser_.set_sk(cyng::value_cast<scramble_key>(frame.at(0), scramble_key::null_scramble_key_).key());
			});

			//
			//	reset parser before reconnect
			//
			vm_.register_function("ipt.reset.parser", 1, [this](cyng::context& ctx) {
				const cyng::vector_t frame = ctx.get_frame();
				CYNG_LOG_TRACE(logger_, "reset parser "
					<< cyng::value_cast<scramble_key>(frame.at(0), scramble_key::null_scramble_key_));

				//	set new scramble key
				parser_.reset(cyng::value_cast<scramble_key>(frame.at(0), scramble_key::null_scramble_key_).key());
			});
			

			//
			//	register ipt request handler
			//
			vm_.register_function("ipt.start", 0, [this](cyng::context& ctx) {
				this->start();
			});

			//
			//	login response
			//
			vm_.register_function("ipt.res.login.public", 4, std::bind(&bus::ipt_res_login, this, std::placeholders::_1, false));
			vm_.register_function("ipt.res.login.scrambled", 4, std::bind(&bus::ipt_res_login, this, std::placeholders::_1, true));

			vm_.register_function("ipt.res.register.push.target", 4, std::bind(&bus::ipt_res_register_push_target, this, std::placeholders::_1));
			vm_.register_function("ipt.res.deregister.push.target", 4, std::bind(&bus::ipt_res_deregister_push_target, this, std::placeholders::_1));
			vm_.register_function("ipt.req.register.push.target", 5, std::bind(&bus::ipt_req_register_push_target, this, std::placeholders::_1));
			vm_.register_function("ipt.req.deregister.push.target", 3, std::bind(&bus::ipt_req_deregister_push_target, this, std::placeholders::_1));
			vm_.register_function("ipt.res.open.push.channel", 8, std::bind(&bus::ipt_res_open_channel, this, std::placeholders::_1));
			vm_.register_function("ipt.res.close.push.channel", 4, std::bind(&bus::ipt_res_close_channel, this, std::placeholders::_1));
			vm_.register_function("ipt.res.transfer.pushdata", 7, std::bind(&bus::ipt_res_transfer_push_data, this, std::placeholders::_1));
			vm_.register_function("ipt.req.transmit.data", 1, std::bind(&bus::ipt_req_transmit_data, this, std::placeholders::_1));
			vm_.register_function("ipt.req.open.connection", 1, std::bind(&bus::ipt_req_open_connection, this, std::placeholders::_1));
			vm_.register_function("ipt.res.open.connection", 2, std::bind(&bus::ipt_res_open_connection, this, std::placeholders::_1));
			vm_.register_function("ipt.req.close.connection", 2, std::bind(&bus::ipt_req_close_connection, this, std::placeholders::_1));
			vm_.register_function("ipt.res.close.connection", 3, std::bind(&bus::ipt_res_close_connection, this, std::placeholders::_1));

			vm_.register_function("ipt.req.protocol.version", 2, std::bind(&bus::ipt_req_protocol_version, this, std::placeholders::_1));
			vm_.register_function("ipt.req.software.version", 2, std::bind(&bus::ipt_req_software_version, this, std::placeholders::_1));
			vm_.register_function("ipt.req.device.id", 2, std::bind(&bus::ipt_req_device_id, this, std::placeholders::_1));
			vm_.register_function("ipt.req.net.stat", 2, std::bind(&bus::ipt_req_net_stat, this, std::placeholders::_1));
			vm_.register_function("ipt.req.ip.statistics", 2, std::bind(&bus::ipt_req_ip_statistics, this, std::placeholders::_1));
			vm_.register_function("ipt.req.dev.auth", 2, std::bind(&bus::ipt_req_dev_auth, this, std::placeholders::_1));
			vm_.register_function("ipt.req.dev.time", 2, std::bind(&bus::ipt_req_dev_time, this, std::placeholders::_1));
			vm_.register_function("ipt.req.transfer.pushdata", 7, std::bind(&bus::ipt_req_transfer_pushdata, this, std::placeholders::_1));

			vm_.register_function("bus.store.relation", 2, std::bind(&bus::store_relation, this, std::placeholders::_1));
			vm_.register_function("bus.remove.relation", 1, std::bind(&bus::remove_relation, this, std::placeholders::_1));

			//
			//	statistical data
			//
			vm_.async_run(cyng::generate_invoke("log.msg.debug", cyng::invoke("lib.size"), " callbacks registered"));

		}

		void bus::start()
		{
			CYNG_LOG_TRACE(logger_, "start ipt client");
			transition(STATE_INITIAL_);
            do_read();
		}

        void bus::stop()
        {
            //
            //  update state
            //
			if (transition(STATE_SHUTDOWN_))	{
				state_ = STATE_SHUTDOWN_;

				//
				//	stop all runing tasks
				//
				mux_.size([this](std::size_t size) {
					CYNG_LOG_INFO(logger_, "ipt bus "
						<< vm_.tag()
						<< " stops "
						<< task_db_.size()
						<< '/'
						<< size
						<< " task(s)");
				});
				for (auto const& tsk : task_db_) {
					mux_.stop(tsk.second);
				}
				

				//
				//  close socket
				//
				boost::system::error_code ec;
				socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
				socket_.close(ec);

				//
				//  no more callbacks
				//
				vm_.halt();
			}
			else
			{
				CYNG_LOG_WARNING(logger_, "ipt bus "
				<< vm_.tag()
				<< " already in shutdown mode");
			}
        }

		bool bus::is_online() const
		{
			switch (state_.load()) {
			case STATE_AUTHORIZED_:
			case STATE_CONNECTED_:
			case STATE_WAIT_FOR_OPEN_RESPONSE_:
			case STATE_WAIT_FOR_CLOSE_RESPONSE_:
				return true;
			default:
				break;
			}
			return false;
		}

		bool bus::is_connected() const
		{
			return STATE_CONNECTED_ == state_.load();
		}

		bool bus::has_watchdog() const
		{
			return watchdog_ != 0u;
		}

		std::uint16_t bus::get_watchdog() const
		{
			return watchdog_;
		}

		boost::asio::ip::tcp::endpoint bus::local_endpoint() const
		{
			return socket_.local_endpoint();
		}

		boost::asio::ip::tcp::endpoint bus::remote_endpoint() const
		{
			return socket_.remote_endpoint();
		}

		void bus::do_read()
		{
            //
            //  do nothing during shutdown
            //
            if (STATE_SHUTDOWN_ == state_.load())  return;

			//auto self(shared_from_this());
			BOOST_ASSERT(socket_.is_open());
			socket_.async_read_some(boost::asio::buffer(buffer_),
				[this](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					//CYNG_LOG_TRACE(logger_, bytes_transferred << " bytes read");"log.hex.dump"
					vm_.async_run(cyng::generate_invoke("log.msg.trace", "ipt client received ", bytes_transferred, cyng::invoke("log.fmt.byte")));
					auto const buffer = parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);
					//auto const prg = cyng::generate_invoke("log.msg.trace", "ipt client received ", buffer, cyng::invoke("log.hex.dump"));
					vm_.async_run(cyng::generate_invoke("log.msg.trace", buffer, cyng::invoke("log.hex.dump")));

#ifdef SMF_IO_DEBUG
					cyng::io::hex_dump hd;
					std::stringstream ss;
					hd(ss, buffer.begin(), buffer.end());
					CYNG_LOG_TRACE(logger_, "ipt input dump \n" << ss.str());
#endif

					do_read();
				}
				else if (ec != boost::asio::error::operation_aborted)
				{
					CYNG_LOG_WARNING(logger_, vm_.tag() << " lost connection <" << ec << ':' << ec.value() << ':' << ec.message() << '>');
					state_ = STATE_ERROR_;

					//
					//	slot [1] - go offline
					//
					mux_.stop("node::watchdog");
					on_logout();
				}
				else
				{
					CYNG_LOG_WARNING(logger_, vm_.tag() << " lost connection intentionally <" << ec << ':' << ec.value() << ':' << ec.message() << '>');
					//
					//	The connection was closed intentionally.
					//
					state_ = STATE_INITIAL_;
				}
			});

		}

		void bus::ipt_res_login(cyng::context& ctx, bool scrambled)
		{
			BOOST_ASSERT(vm_.tag() == ctx.tag());

			cyng::vector_t const frame = ctx.get_frame();

            //
            //  * [uuid] ident
            //  * [uin8] response
            //  * [uin16] watchdog
            //  * [string] redirect
            //

			//
			//	same for public and scrambled
			//
			const auto res = make_login_response(cyng::value_cast(frame.at(1), response_type(0)));

			//
			//	update watchdog
			//
			watchdog_ = cyng::value_cast(frame.at(2), std::uint16_t(23));
			const std::string redirect = cyng::value_cast<std::string>(frame.at(3), "");

			if (scrambled) {
				ctx.queue(cyng::generate_invoke("log.msg.info", "ipt.res.login.scrambled ", res.get_response_name()));
			}
			else	{
				ctx.queue(cyng::generate_invoke("log.msg.info", "ipt.res.login.public ", res.get_response_name()));
			}

			if (res.is_success())
			{
				//
				//	update bus state
				//
				state_ = STATE_AUTHORIZED_;

				ctx.queue(cyng::generate_invoke("log.msg.info"
					, "successful authorized(local: "
					, socket_.local_endpoint()
					, ", remote: "
					, socket_.remote_endpoint()));

				//
				//	slot [0]
				//	Only successful logins will be posted. In case of failure the on_logout()
				//	event will be called.
				//
				if (has_watchdog()) {

					//
					//	start watchdog task
					//
					cyng::async::start_task_sync<watchdog>(mux_, logger_, vm_, watchdog_);

				}
				on_login_response(watchdog_, redirect);
			}
			else	{
				state_ = STATE_ERROR_;
				watchdog_ = 0u;
                ctx.queue(cyng::generate_invoke("log.msg.warning", "login failed"));

				//
				//	slot [1]
				//
				on_logout();
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

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] session tag
				sequence_type,			//	[1] ipt seq
				response_type,			//	[2] ipt response
				std::uint32_t			//	[3] channel
			>(frame);

			const response_type res = std::get<2>(tpl);

			//
			//	search task
			//
			auto pos = task_db_.find(std::get<1>(tpl));
			if (pos != task_db_.end())
			{
				const auto tsk = pos->second;

				CYNG_LOG_DEBUG(logger_, "ipt.res.register.push.target "
					<< +pos->first
					<< " => #"
					<< tsk);

				mux_.post(tsk, 0, cyng::tuple_factory(std::get<1>(tpl), ctrl_res_register_target_policy::is_success(res), std::get<3>(tpl)));

				//
				//	remove entry
				//
				task_db_.erase(pos);
			}
			else
			{
				auto msg = ctrl_res_register_target_policy::get_response_name(res);
				CYNG_LOG_WARNING(logger_, "ipt.res.register.push.target - no task entry for ipt channel "
					<< +std::get<1>(tpl)
					<< " => "
					<< std::get<3>(tpl)
					<< " - "
					<< msg);
			}
		}

		void bus::ipt_res_deregister_push_target(cyng::context& ctx)
		{
			//	[aab5764f-fc49-4bdf-ac6b-5abab1491846,2,1,power@solostec]
			//
			//	* session tag
			//	* seq
			//	* response
			//	* target name
			//
			const cyng::vector_t frame = ctx.get_frame();

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] session tag
				sequence_type,		//	[1] ipt seq
				response_type,		//	[2] ipt response
				std::string			//	[3] target
			>(frame);

			const response_type res = std::get<2>(tpl);

			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", ctrl_res_deregister_target_policy::get_response_name(res)));

			//
			//	* [u8] seq
			//	* [bool] success
			//	* [string] target
			//
			on_res_deregister_target(std::get<1>(tpl)
				, ctrl_res_deregister_target_policy::is_success(res)
				, std::get<3>(tpl));
		}

		void bus::ipt_req_register_push_target(cyng::context& ctx)
		{
			//
			//	* [uuid] session tag
			//	* [u8] seq
			//	* [string] target
			//	* [u16] packet size
			//	* [u8] window size
			//
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

			//
			//	get sequence
			//
			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] session tag
				sequence_type,			//	[1] ipt seq
				std::string,			//	[2] target
				std::uint16_t,			//	[3] packet size
				std::uint8_t			//	[4] window size
			>(frame);

			BOOST_ASSERT_MSG(false, "ipt.req.register.push.target");
		}

		void bus::ipt_req_deregister_push_target(cyng::context& ctx)
		{
			//
			//	* [uuid] session tag
			//	* [u8] seq
			//	* [string] target
			//
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

			//
			//	get sequence
			//
			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] session tag
				sequence_type,			//	[1] ipt seq
				std::string				//	[2] target
			>(frame);

			BOOST_ASSERT_MSG(false, "ipt.req.deregister.push.target");
		}

		void bus::ipt_res_open_channel(cyng::context& ctx)
		{
			//
			//	* session tag
			//	* seq
			//	* response
			//	* channel
			//	* source
			//	* packet size
			//	* window size
			//	* status (reserved)
			//	* target count
			//
			const cyng::vector_t frame = ctx.get_frame();

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] session tag
				sequence_type,		//	[1] ipt seq
				response_type,		//	[2] ipt response
				std::uint32_t,		//	[3] channel
				std::uint32_t,		//	[4] source
				std::uint16_t,		//	[5] packet size
				std::uint8_t,		//	[6] window size
				std::uint8_t,		//	[7] status (reserved)
				std::size_t			//	[8] count
			>(frame);

			response_type const res = std::get<2>(tpl);
			
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), "(channel: "
				, frame.at(3)
				, ", source: "
				, frame.at(4)
				, ", res: "
				, tp_res_open_push_channel_policy::get_response_name(res)));

			//
			//	check packet size
			//
			BOOST_ASSERT_MSG(std::get<5>(tpl) != 0, "invalid packet size");

			BOOST_ASSERT_MSG(std::get<7>(tpl) == 0, "status is reserved and must be zero");

			//
			//	test window size
			//
			if (std::get<6>(tpl) != 1) {
				ctx.queue(cyng::generate_invoke("log.msg.warning", ctx.get_name(), "(channel: "
					, frame.at(3)
					, ", source: "
					, frame.at(4)
					, ", has invalid window size: "
					, std::get<6>(tpl)));
			}

			//
			//	lookup in task db
			//
			auto pos = task_db_.find(std::get<1>(tpl));
			if (pos != task_db_.end())
			{
				const auto tsk = pos->second;

				CYNG_LOG_DEBUG(logger_, ctx.get_name()
					<< " "
					<< +pos->first
					<< " => #"
					<< tsk);

				mux_.post(tsk, 0
					//	[bool] success
					, cyng::tuple_factory(tp_res_open_push_channel_policy::is_success(res)
					, std::get<3>(tpl)	//	[u32] channel
					, std::get<4>(tpl)	//	[u32] source
					, std::get<7>(tpl)	//	[u16] status
					, std::get<8>(tpl)	//	[size] count
				));	

				//
				//	remove entry
				//
				task_db_.erase(pos);

				//
				//	manage channel db
				//	channel => packet size
				//
				channel_db_.emplace(std::get<3>(tpl), std::get<5>(tpl));
			}
			else
			{
				auto msg = ctrl_res_register_target_policy::get_response_name(res);
				CYNG_LOG_WARNING(logger_, "ipt.res.register.push.target - no task entry for ipt channel "
					<< +std::get<1>(tpl)
					<< " => "
					<< std::get<3>(tpl)
					<< " - "
					<< msg);
			}
		}

		void bus::ipt_res_close_channel(cyng::context& ctx)
		{
			//
			//	* session tag
			//	* seq
			//	* response
			//	* channel
			//
			const cyng::vector_t frame = ctx.get_frame();

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] session tag
				sequence_type,		//	[1] ipt seq
				response_type,		//	[2] ipt response
				std::uint32_t
			>(frame);

			response_type const res = std::get<2>(tpl);

			ctx.queue(cyng::generate_invoke("log.msg.trace"
				, ctx.get_name()
				, " - "
				, std::get<3>(tpl)
				, ": "
				, tp_res_close_push_channel_policy::get_response_name(res)));


			// 
			//	* [u8] seq
			//	* [bool] success
			//	* [u32] channel
			//	* [string] response name
			//	
			on_res_close_push_channel(std::get<1>(tpl)
				, tp_res_close_push_channel_policy::is_success(res)
				, std::get<3>(tpl));
		}

		void bus::ipt_res_transfer_push_data(cyng::context& ctx)
		{
			//	[0fa1b1e9-ab1c-4b15-b7d7-f0ab23d8a90b,2,1,107f6cde,88c9f426,20,1]

			//	[uuid] ident
			//	[u8] sequence
			//	[u8] response code
			//	[u32] channel id
			//	[u32] source id (session tag)
			//	[u8] status
			//	[u8] block
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] session tag
				sequence_type,		//	[1] ipt seq
				response_type,		//	[2] ipt response
				std::uint32_t,		//	[3] channel
				std::uint32_t,		//	[4] source
				std::uint8_t,		//	[5] status
				std::uint8_t		//	[6] block
			>(frame);

			response_type const res = std::get<2>(tpl);

			//
			//	lookup in task db
			//
			auto pos = task_db_.find(std::get<1>(tpl));
			if (pos != task_db_.end())
			{
				const auto tsk = pos->second;

				CYNG_LOG_DEBUG(logger_, ctx.get_name()
					<< " "
					<< +pos->first
					<< " => #"
					<< tsk);

				mux_.post(tsk, 0
					//	[bool] success
					, cyng::tuple_factory(tp_res_pushdata_transfer_policy::is_success(res)
						, std::get<3>(tpl)	//	[u32] channel
						, std::get<4>(tpl)	//	[u32] source
						, std::get<5>(tpl)	//	[u8] status
						, std::get<6>(tpl)	//	[u8] block
					));

				//
				//	remove entry
				//
				task_db_.erase(pos);

			}
			else
			{
				auto msg = tp_res_pushdata_transfer_policy::get_response_name(res);
				CYNG_LOG_WARNING(logger_, "ipt.res.transfer.pushdata - no task entry for ipt channel "
					<< +std::get<3>(tpl)
					<< ":"
					<< std::get<4>(tpl)
					<< " - "
					<< msg);
			}
		}

		void bus::ipt_req_transmit_data(cyng::context& ctx)
		{
			//	[0b5d8da4-ce8d-4c4f-bb02-9a9f173391d4,1B1B1B1B010101017681063...2007101633789000000001B1B1B1B1A034843]
			const cyng::vector_t frame = ctx.get_frame();

			auto bp = cyng::object_cast<cyng::buffer_t>(frame.at(1));
			BOOST_ASSERT_MSG(bp != nullptr, "no data");
			if (STATE_CONNECTED_ != state_) {
				ctx.queue(cyng::generate_invoke("log.msg.warning", "ipt.req.transmit.data - wrong state: ", get_state()));
			}

			if (bp != nullptr) {
				ctx.queue(cyng::generate_invoke("log.msg.trace", "ipt.req.transmit.data ", bp->size()));
				auto buffer = on_transmit_data(*bp);
				if (!buffer.empty()) {
					ctx.queue(cyng::generate_invoke("ipt.transfer.data", std::move(buffer)));
					ctx.queue(cyng::generate_invoke("stream.flush"));
				}
			}
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

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,	//	[0] session tag
				sequence_type,		//	[1] ipt seq
				std::string			//	[2] number
			>(frame);

			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame, ", state: ", get_state()));

			switch (state_.load())
			{
			case STATE_AUTHORIZED_:
				//
				//	[u8] seq
				//	[string] number
				//
				if (on_req_open_connection(std::get<1>(tpl), std::get<2>(tpl))) {

					//
					//	update internal state
					//
					transition(STATE_CONNECTED_);

					//
					//	accept calls
					//
					ctx.queue(cyng::generate_invoke("res.open.connection"
						, std::get<1>(tpl)
						, static_cast<response_type>(ipt::tp_res_open_connection_policy::DIALUP_SUCCESS)));
				}
				else {

					//
					//	dismiss calls
					//
					ctx.queue(cyng::generate_invoke("res.open.connection"
						, std::get<1>(tpl)
						, static_cast<response_type>(ipt::tp_res_open_connection_policy::BUSY)));
				}
				ctx.queue(cyng::generate_invoke("stream.flush"));
				break;
			case STATE_CONNECTED_:
				//	busy
				ctx	.queue(cyng::generate_invoke("res.open.connection", frame.at(1), static_cast<std::uint8_t>(ipt::tp_res_open_connection_policy::BUSY)))
					.queue(cyng::generate_invoke("stream.flush"));
				break;
			default:
				ctx	.queue(cyng::generate_invoke("res.open.connection", frame.at(1), static_cast<std::uint8_t>(ipt::tp_res_open_connection_policy::DIALUP_FAILED)))
					.queue(cyng::generate_invoke("stream.flush"));
				break;
			}
		}

		void bus::ipt_res_open_connection(cyng::context& ctx)
		{
			//	[32c4de22-b9ce-4f72-8680-5c047fb6698d,1,1]
			//
			//	* session tag
			//	* [u8] ipt sequence 
			//	* [u8] responce
			//
			const cyng::vector_t frame = ctx.get_frame();

			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] session tag
				sequence_type,			//	[1] ipt seq
				response_type			//	[2] ipt response
			>(frame);

			const auto success = tp_res_open_connection_policy::is_success(std::get<2>(tpl));
			const auto msg = tp_res_open_connection_policy::get_response_name(std::get<2>(tpl));

			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), ": ", msg));

			auto pos = task_db_.find(std::get<1>(tpl));
			if (pos != task_db_.end())
			{
				const auto tsk = pos->second;
				BOOST_ASSERT(std::get<1>(tpl) == pos->first);

				CYNG_LOG_DEBUG(logger_, "ipt.res.open.connection "
					<< +pos->first
					<< " => #"
					<< tsk);

				switch (state_.load())
				{
				case STATE_WAIT_FOR_OPEN_RESPONSE_:

					//
					//	update task state
					//
					transition(STATE_CONNECTED_);

					//
					//	post to task <open_response>
					//
					mux_.post(tsk, 0, cyng::tuple_factory(std::get<1>(tpl), success));

					break;
				case STATE_CONNECTED_:
					ctx.queue(cyng::generate_invoke("log.msg.warning", "ipt.res.open.connection already connected ", frame));
					mux_.post(tsk, 0, cyng::tuple_factory(std::get<2>(tpl), false));
					break;
				default:
					ctx.queue(cyng::generate_invoke("log.msg.warning", "ipt.res.open.connection in wrong state ", get_state()));
					state_ = STATE_ERROR_;
					mux_.post(tsk, 0, cyng::tuple_factory(std::get<2>(tpl), false));
					break;
				}

				//
				//	remove entry
				//
				task_db_.erase(pos);
			}
			else	{
				state_ = (STATE_WAIT_FOR_OPEN_RESPONSE_ == state_)
					? STATE_AUTHORIZED_
					: STATE_ERROR_
					;
				ctx.queue(cyng::generate_invoke("log.msg.warning", "ipt.res.open.connection: empty sequence/task relation (timeout) ", +std::get<1>(tpl)));
			}
		}

		void bus::ipt_req_close_connection(cyng::context& ctx)
		{
			//	[4e645b8d-4eda-46a5-84b4-c1fa182e8247,9]
			//
			//	* session tag
			//	* ipt sequence 
			//
			const cyng::vector_t frame = ctx.get_frame();

			switch (state_.load())
			{
			case STATE_AUTHORIZED_:
				ctx.queue(cyng::generate_invoke("log.msg.warning", "received ipt.req.close.connection - not connected ", frame));
				break;
			case STATE_CONNECTED_:
				ctx.queue(cyng::generate_invoke("log.msg.debug", "received ipt.req.close.connection ", frame));

				//
				//	update connection state
				//	no action from client required
				//
				if (transition(STATE_AUTHORIZED_))	{
					const auto seq = cyng::value_cast<sequence_type>(frame.at(1), 0u);

					//	
					//	* [u8] seq
					//
					on_req_close_connection(seq);
				}
				break;
			case STATE_WAIT_FOR_CLOSE_RESPONSE_:
				ctx.queue(cyng::generate_invoke("log.msg.error", "received ipt.req.close.connection - invalid state: ", get_state()));
				state_ = STATE_AUTHORIZED_;	//	try to fix this 
				break;
			default:
				ctx.queue(cyng::generate_invoke("log.msg.error", "received ipt.req.close.connection - invalid state: ", get_state()));
				break;
			}

			//
			//	accept request in every case - send response
			//
			ctx	.queue(cyng::generate_invoke("res.close.connection"
					, frame.at(1)	//	ip-t seq
					, static_cast<std::uint8_t>(ipt::tp_res_close_connection_policy::CONNECTION_CLEARING_SUCCEEDED)))	//	OK
				.queue(cyng::generate_invoke("stream.flush"));

		}

		void bus::ipt_res_close_connection(cyng::context& ctx)
		{
			//
			//	* [uuid] session tag
			//	* [u8] seq
			//	* [u8] response
			//
			const cyng::vector_t frame = ctx.get_frame();
			ctx.queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame));

			//
			//	get sequence
			//
			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] session tag
				sequence_type,			//	[1] ipt seq
				response_type			//	[2] ipt response
			>(frame);

			const auto seq = std::get<1>(tpl);
			const auto success = tp_res_close_connection_policy::is_success(std::get<2>(tpl));

			//
			//	search for origin task
			//
			auto pos = task_db_.find(seq);
			if (pos != task_db_.end()) {

				//	origin task
				const auto tsk = pos->second;

				if (STATE_WAIT_FOR_CLOSE_RESPONSE_ == state_) {

					//
					//	update bus state
					//
					transition(STATE_AUTHORIZED_);

				}
				else {

					//
					//	error
					//
					state_ = STATE_ERROR_;
					CYNG_LOG_WARNING(logger_, ctx.tag()
						<< " received a connection close response but didn't wait for one: "
						<< get_state());
				}

				//
				//	stop task <close_connection>
				//	and forward connection close response to client
				//
				CYNG_LOG_INFO(logger_, ctx.tag() << " - ipt.res.close.connection - stop task #" << tsk);
				mux_.post(tsk, 0, cyng::tuple_factory(seq, success));

				//
				//	remove entry
				//
				task_db_.erase(pos);
			}
			else {
				CYNG_LOG_WARNING(logger_, ctx.tag() << "ipt.res.close.connection: no entry in task db for seq " << +seq);
			}
		}


		void bus::ipt_req_protocol_version(cyng::context& ctx)
		{
			BOOST_ASSERT(vm_.tag() == ctx.tag());
			const cyng::vector_t frame = ctx.get_frame();
			ctx	.queue(cyng::generate_invoke("log.msg.debug", ctx.get_name(), " - ", frame))
				.queue(cyng::generate_invoke("res.protocol.version", frame.at(1), static_cast<std::uint8_t>(1)))
				.queue(cyng::generate_invoke("stream.flush"));
		}

		void bus::ipt_req_software_version(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx. queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame))
				.queue(cyng::generate_invoke("res.software.version", frame.at(1), NODE_VERSION))
				.queue(cyng::generate_invoke("stream.flush"));
		}

		void bus::ipt_req_device_id(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx. queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame))
				.queue(cyng::generate_invoke("res.device.id", frame.at(1), model_))
				.queue(cyng::generate_invoke("stream.flush"));
		}

		void bus::ipt_req_net_stat(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx. queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame))
				.queue(cyng::generate_invoke("res.unknown.command", frame.at(1), static_cast<std::uint16_t>(code::APP_REQ_NETWORK_STATUS)))
				.queue(cyng::generate_invoke("stream.flush"));
		}

		void bus::ipt_req_ip_statistics(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx. queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame))
				.queue(cyng::generate_invoke("res.unknown.command", frame.at(1), static_cast<std::uint16_t>(code::APP_REQ_IP_STATISTICS)))
				.queue(cyng::generate_invoke("stream.flush"));
		}

		void bus::ipt_req_dev_auth(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx. queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame))
				.queue(cyng::generate_invoke("res.unknown.command", frame.at(1), static_cast<std::uint16_t>(code::APP_REQ_DEVICE_AUTHENTIFICATION)))
				.queue(cyng::generate_invoke("stream.flush"));
		}

		void bus::ipt_req_dev_time(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx. queue(cyng::generate_invoke("log.msg.trace", ctx.get_name(), " - ", frame))
				.queue(cyng::generate_invoke("res.device.time", frame.at(1)))
				.queue(cyng::generate_invoke("stream.flush"));
		}

		void bus::ipt_req_transfer_pushdata(cyng::context& ctx)
		{
			//	[7aee3dff-c81b-414e-a5dc-4e3dbe4122f5,9,fb2a0137,a1e24bba,c1,0,1B1B1B1B010101017606363939373462006200726301017601080500153B0223B3063639393733080500153B0223B3010163C4F400]
			//
			//	* session tag
			//	* sequence
			//	* channel
			//	* source
			//	* status
			//	* block
			//	* data
			const cyng::vector_t frame = ctx.get_frame();
			auto const tpl = cyng::tuple_cast<
				boost::uuids::uuid,		//	[0] session tag
				sequence_type,			//	[1] ipt seq
				std::uint32_t,			//	[2] channel
				std::uint32_t,			//	[3] source
				std::uint8_t,			//	[4] status
				std::uint8_t,			//	[5] block
				cyng::buffer_t			//	[6] data
			>(frame);

			ctx.queue(cyng::generate_invoke("log.msg.debug", "ipt.req.transfer.pushdata(channel: "
				, std::get<2>(tpl)
				, ", source: "
				, std::get<3>(tpl)
				, " "
				, std::get<6>(tpl).size()
				, cyng::invoke("log.fmt.byte")));


#ifdef SMF_IO_DEBUG

			std::ofstream of("push-data-" + std::to_string(std::get<2>(tpl)) + "-" + std::to_string(std::get<3>(tpl)) + ".bin"
			, std::ios::out | std::ios::binary | std::ios::app);
			if (of.is_open())
			{
				of.write(std::get<6>(tpl).data(), std::get<6>(tpl).size());
			}
#endif
			// 
			//	* [u8] seq
			//	* [u32] channel
			//	* [u32] source
			//	* [buffer] data
			//
			on_req_transfer_push_data(std::get<1>(tpl) //	seq
				, std::get<2>(tpl)		//	channel
				, std::get<3>(tpl)		//	source
				, std::get<6>(tpl));	//	data
		}

		bool bus::req_login(master_record const& rec)
		{
			if (!transition(STATE_INITIAL_)) {
				CYNG_LOG_WARNING(logger_, rec.account_ 
					<< '@' 
					<< vm_.tag() 
					<< " already authorized: " 
					<< get_state());
				return false;
			}

			if (rec.scrambled_) {
				CYNG_LOG_INFO(logger_, vm_.tag() 
					<< " send scrambled login request: " 
					<< rec.account_
					<< '@'
					<< rec.host_
					<< ':'
					<< rec.service_);

				vm_.async_run(ipt::gen::ipt_req_login_scrambled(rec));
			}
			else {

				CYNG_LOG_INFO(logger_, vm_.tag()
					<< " send public login request: "
					<< rec.account_
					<< '@'
					<< rec.host_
					<< ':'
					<< rec.service_);

				vm_.async_run(ipt::gen::ipt_req_login_public(rec));
			}
			return true;
		}

		bool bus::req_connection_open(std::string const& number, std::chrono::seconds d)
		{
			//
			//	update state
			//
			if (transition(STATE_WAIT_FOR_OPEN_RESPONSE_)) {

				//
				//	start monitor tasks with N retries
				//
				cyng::async::start_task_sync<open_connection>(mux_, logger_, vm_, number, d, retries_, *this);
				return true;
			}

			CYNG_LOG_WARNING(logger_, vm_.tag() << " try to open connection but is not authorized: " << get_state());
			return false;
		}

		bool bus::req_connection_close(std::chrono::seconds d)
		{
			//
			//	update state
			//
			if (transition(STATE_WAIT_FOR_CLOSE_RESPONSE_)) {

				//
				//	start monitor tasks
				//
				cyng::async::start_task_sync<close_connection>(mux_, logger_, vm_, d, *this);

				return true;
			}

			CYNG_LOG_WARNING(logger_, vm_.tag() << " try to close connection but is not connected: " << get_state());
			return false;
		}

		void bus::res_connection_open(sequence_type seq, bool accept)
		{
			const response_type res = (accept)
				? ipt::tp_res_open_connection_policy::DIALUP_SUCCESS
				: ipt::tp_res_open_connection_policy::DIALUP_FAILED
				;

			if (accept && (STATE_AUTHORIZED_ == state_)) {
				//
				//	update task state
				//
				state_ = STATE_CONNECTED_;
			}

			vm_	.async_run(cyng::generate_invoke("res.open.connection", seq, res))
				.async_run(cyng::generate_invoke("stream.flush"));

		}

		void bus::req_register_push_target(std::string const& name
			, std::uint16_t packet_size
			, std::uint8_t window_size
			, std::chrono::seconds d)
		{
			if (is_online()) {

				//
				//	start monitor tasks
				//
				cyng::async::start_task_sync<register_target>(mux_, logger_, vm_, name, packet_size, window_size, d, *this);

			}
			else {
				CYNG_LOG_WARNING(logger_, vm_.tag() << " try to register target but is not authorized: " << get_state());
			}
		}

		bool bus::req_channel_open(std::string const& target
			, std::string const& account
			, std::string const& msisdn
			, std::string const& version
			, std::string const& device
			, std::uint16_t time_out
			, std::size_t tsk)
		{
			if (is_online()) {

				//
				//	start monitor tasks
				//
				cyng::async::start_task_sync<open_channel>(mux_
					, logger_
					, vm_
					, target
					, account
					, msisdn
					, version
					, device
					, time_out
					, *this
					, tsk);

				return true;
			}

			CYNG_LOG_WARNING(logger_, vm_.tag() << " try to open channel but is not authorized: " << get_state());
			return false;
		}

		bool bus::req_channel_close(std::uint32_t channel)
		{
			//
			//	manage channel db
			//
			channel_db_.erase(channel);

			//
			//	send channel close request - if online
			//
			if (is_online()) {
				vm_.async_run({ cyng::generate_invoke("req.close.push.channel", channel)
								, cyng::generate_invoke("stream.flush")
								, cyng::generate_invoke("log.msg.info", "send req.close.push.channel(seq: ", cyng::invoke("ipt.seq.push"), ", channel: ", channel, ")") });				
				return true;
			}
			return false;
		}

		channel_response bus::req_transfer_push_data(std::uint32_t channel
			, std::uint32_t source
			, cyng::buffer_t const& data
			, std::size_t tsk)
		{
			if (is_online()) {

				//
				//	check package size
				//
				auto const r = test_channel_size(channel, data.size());
				if (r.second) {

					std::uint8_t const status{ 0xc0 };	//	set FIN/SYN flag 

					//
					//	serialize and send push data immediately
					//
					vm_.async_run({
						cyng::generate_invoke("req.transfer.push.data"
							, channel	//	channel
							, source	//	source
							, status
							, static_cast<std::uint8_t>(1)	//	block
							, data),
						cyng::generate_invoke("stream.flush")
						});
					mux_.post(tsk, 3u, cyng::tuple_factory(true, channel, source, static_cast<std::uint8_t>(1)));
					return channel_response::SEND_OK;
				}
				else {

					//
					//	start a task to send the full message
					//
					cyng::async::start_task_sync<transfer_data>(mux_
						, logger_
						, vm_
						, channel	//	channel
						, source	//	source
						, data
						, r.first
						, *this
						, tsk);

					return channel_response::SEND_CHUNCKED;
				}
			}
			return channel_response::SEND_FAILED;
		}

		std::pair<std::uint16_t, bool> bus::test_channel_size(std::uint32_t channel, std::size_t size) const
		{
			auto const pos = channel_db_.find(channel);
			if (pos != channel_db_.end()) {
				return std::pair{ pos->second, pos->second >= size };
			}

			//
			//	channel not found
			//	‭65535‬ is the max size for a data package
			//
			return std::pair{ 0xFFFF, size <= 0xFFFF };
			//return std::pair{ 0xFF, false };
		}

		void bus::store_relation(cyng::context& ctx)
		{
			//	[1,2]
			//
			//	* ipt sequence number
			//	* task id
			//	
			const cyng::vector_t frame = ctx.get_frame();
			auto const tpl = cyng::tuple_cast<
				sequence_type,		//	[0] ipt seq
				std::size_t			//	[1] task id
			>(frame);

			if (task_db_.find(std::get<0>(tpl)) != task_db_.end()) {

				CYNG_LOG_WARNING(logger_, "bus.store.relation - slot "
					<< +std::get<0>(tpl)
					<< " already occupied with #"
					<< std::get<1>(tpl)
					<< '/'
					<< task_db_.size());
			}

			//
			//	store seq => task relation
			//
			task_db_.emplace(std::get<0>(tpl), std::get<1>(tpl));

			CYNG_LOG_INFO(logger_, "bus.store.relation "
				<< +std::get<0>(tpl)
				<< " => #"
				<< std::get<1>(tpl)
				<< '/'
				<< task_db_.size());

			//
			//	update ipt sequence
			//
			mux_.post(std::get<1>(tpl), 1u, cyng::tuple_factory(std::get<0>(tpl)));
		}

		void bus::remove_relation(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			auto const tpl = cyng::tuple_cast<
				std::size_t			//	[0] task id
			>(frame);

			for (auto pos = task_db_.begin(); pos != task_db_.end(); ++pos) {
				if (pos->second == std::get<0>(tpl)) {

					CYNG_LOG_DEBUG(logger_, "bus.remove.relation "
						<< +pos->first
						<< " => #"
						<< pos->second);

					//
					//	remove from task db
					//
					task_db_.erase(pos);

					break;
				}
			}
		}

		std::string bus::get_state() const
		{
			switch (state_) {
			case STATE_INITIAL_:	return "INITIAL";
			case STATE_AUTHORIZED_:	return "AUTHORIZED";
			case STATE_CONNECTED_:	return "CONNECTED";
			case STATE_WAIT_FOR_OPEN_RESPONSE_:	return "WAIT_FOR_OPEN_RESPONSE";
			case STATE_WAIT_FOR_CLOSE_RESPONSE_:	return "WAIT_FOR_CLOSE_RESPONSE";
			case STATE_SHUTDOWN_:	return "SHUTDOWN";
			case STATE_HALTED_:		return "HALTED";
			default:
				break;
			}
			return "ERROR";
		}

		bool bus::transition(state evt)
		{
			switch (evt) {

			case STATE_INITIAL_:
				switch (state_) {
				case STATE_INITIAL_:
				case STATE_ERROR_:
					state_.exchange(evt);
					return true;
				default:
					break;
				}
				return false;

			case STATE_SHUTDOWN_:
				if (STATE_SHUTDOWN_ != state_) {
					state_.exchange(evt);
					return true;
				}
				return false;

			case STATE_CONNECTED_:
				switch (state_) {
				case STATE_AUTHORIZED_:
				case STATE_WAIT_FOR_OPEN_RESPONSE_:
					state_.exchange(evt);
					return true;
				default:
					break;
				}
				return false;

			case STATE_AUTHORIZED_:
				switch (state_) {
				case STATE_CONNECTED_:
				case STATE_WAIT_FOR_CLOSE_RESPONSE_:
					state_.exchange(evt);
					return true;
				default:
					break;
				}
				return false;

			case STATE_WAIT_FOR_OPEN_RESPONSE_:
				if (STATE_AUTHORIZED_ == state_) {
					state_.exchange(evt);
					return true;
				}
				return false;

			case STATE_WAIT_FOR_CLOSE_RESPONSE_:
				if (STATE_CONNECTED_ == state_) {
					state_.exchange(evt);
					return true;
				}
				return false;

			default:
				state_.exchange(evt);
				break;
			}
			return true;
		}
	}
}
