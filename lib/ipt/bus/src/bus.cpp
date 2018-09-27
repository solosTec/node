/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/ipt/bus.h>
#include <smf/ipt/scramble_key_io.hpp>
#include <smf/ipt/response.hpp>
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

namespace node
{
	namespace ipt
	{
		bus::bus(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, boost::uuids::uuid tag
			, scramble_key const& sk
			, std::size_t tsk
			, std::string const& model
			, std::size_t retries)
		: vm_(mux.get_io_service(), tag)
			, socket_(mux.get_io_service())
			, buffer_()
			, mux_(mux)
			, logger_(logger)
			, task_(tsk)
			, parser_([this](cyng::vector_t&& prg) {
				CYNG_LOG_DEBUG(logger_, prg.size() << " ipt instructions received");
#ifdef SMF_IO_DEBUG
				CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
#endif
				vm_.async_run(std::move(prg));
			}, sk)
			, serializer_(socket_, vm_, sk)
			, model_(model)
			, retries_((retries == 0) ? 1 : retries)
			, watchdog_(0)
			, state_(STATE_INITIAL_)
			, task_db_()
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
			vm_.register_function("ipt.res.open.push.channel", 8, std::bind(&bus::ipt_res_open_channel, this, std::placeholders::_1));
			vm_.register_function("ipt.res.close.push.channel", 4, std::bind(&bus::ipt_res_close_channel, this, std::placeholders::_1));
			vm_.register_function("ipt.res.transfer.pushdata", 0, std::bind(&bus::ipt_res_transfer_push_data, this, std::placeholders::_1));
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

			//
			//	statistical data
			//
			vm_.async_run(cyng::generate_invoke("log.msg.info", cyng::invoke("lib.size"), "callbacks registered"));

		}

		void bus::start()
		{
			CYNG_LOG_TRACE(logger_, "start ipt client");
            state_ = STATE_INITIAL_;
            do_read();
		}

        void bus::stop()
        {
            //
            //  update state
            //
			if (state_ != STATE_SHUTDOWN_)
			{
				state_ = STATE_SHUTDOWN_;

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
					vm.run(cyng::generate_invoke("log.msg.info", "fast shutdown"));
					vm.run(cyng::vector_t{ cyng::make_object(cyng::code::HALT) });
				});
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
			return state_ == STATE_AUTHORIZED_;
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
            if (state_ == STATE_SHUTDOWN_)  return;

			auto self(shared_from_this());
			BOOST_ASSERT(socket_.is_open());
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
				else if (ec != boost::asio::error::operation_aborted)
				{
					CYNG_LOG_WARNING(logger_, vm_.tag() << " lost connection <" << ec << ':' << ec.value() << ':' << ec.message() << '>');
					state_ = STATE_ERROR_;

					//
					//	slot [1] - go offline
					//
					mux_.post(task_, IPT_EVENT_CONNECTION_TO_MASTER_LOST, cyng::tuple_t());

				}
				else
				{
					//
					//	The connection was closed intentionally.
					//
				}
			});

		}

		void bus::ipt_res_login(cyng::context& ctx, bool scrambled)
		{
			BOOST_ASSERT(vm_.tag() == ctx.tag());

			const cyng::vector_t frame = ctx.get_frame();

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
			watchdog_ = cyng::value_cast(frame.at(2), std::uint16_t(23));
			const std::string redirect = cyng::value_cast<std::string>(frame.at(3), "");

			if (scrambled)
			{
				ctx.attach(cyng::generate_invoke("log.msg.info", "ipt.res.login.scrambled", res.get_response_name()));
			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.info", "ipt.res.login.public", res.get_response_name()));
			}

			if (res.is_success())
			{
				//
				//	update bus state
				//
				state_ = STATE_AUTHORIZED_;

				ctx.attach(cyng::generate_invoke("log.msg.info"
					, "successful authorized"
					, socket_.local_endpoint()
					, socket_.remote_endpoint()));

				//
				//	slot [0]
				//
				mux_.post(task_, IPT_EVENT_AUTHORIZED, cyng::tuple_factory(watchdog_, redirect));

			}
			else
			{
				state_ = STATE_ERROR_;
				watchdog_ = 0u;
                ctx.attach(cyng::generate_invoke("log.msg.warning", "login failed"));

				//
				//	slot [1]
				//
				mux_.post(task_, IPT_EVENT_CONNECTION_TO_MASTER_LOST, cyng::tuple_t());
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

			const response_type res = cyng::value_cast<response_type>(frame.at(2), 0);
			if (ctrl_res_register_target_policy::is_success(res))
			{
				ctx.attach(cyng::generate_invoke("log.msg.info", "client.res.register.push.target", ctrl_res_register_target_policy::get_response_name(res)));
			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.warning", "client.res.register.push.target", ctrl_res_register_target_policy::get_response_name(res)));
			}

			//
			//	* [u8] seq
			//	* [bool] success
			//	* [u32] channel
			//
			mux_.post(task_
				, IPT_EVENT_PUSH_TARGET_REGISTERED
				, cyng::tuple_factory(frame.at(1), ctrl_res_register_target_policy::is_success(res), frame.at(3)));
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
			vm_.async_run(cyng::generate_invoke("log.msg.debug", "ipt.res.deregister.push.target", frame));
			const response_type res = cyng::value_cast<response_type>(frame.at(2), 0);

			//
			//	* [u8] seq
			//	* [bool] success
			//	* [string] target
			//
			mux_.post(task_
				, IPT_EVENT_PUSH_TARGET_DEREREGISTERED
				, cyng::tuple_factory(frame.at(1), ctrl_res_deregister_target_policy::is_success(res), frame.at(3)));
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
			//	* status
			//	* target count
			//
			const cyng::vector_t frame = ctx.get_frame();
			const response_type res = cyng::value_cast<response_type>(frame.at(2), 0);
			
			ctx.attach(cyng::generate_invoke("log.msg.debug", "ipt.res.open.push.channel"
				, frame.at(3)
				, frame.at(4)
				, tp_res_open_push_channel_policy::get_response_name(res)));

			//
			//	* [u8] seq
			//	* [u8] res
			//	* [u32] channel
			//	* [u32] source
			//	* [u16] packet size
			//	* [size] target count
			//	
			mux_.post(task_
				, IPT_EVENT_PUSH_CHANNEL_OPEN
				, cyng::tuple_factory(frame.at(1)
					, tp_res_open_push_channel_policy::is_success(res)
					, frame.at(3)
					, frame.at(4)
					, frame.at(5)
					, frame.at(8)));
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
			const response_type res = cyng::value_cast<response_type>(frame.at(2), 0);

			ctx.attach(cyng::generate_invoke("log.msg.debug"
				, "ipt.res.close.push.channel"
				, frame.at(3)
				, tp_res_close_push_channel_policy::get_response_name(res)));


			// 
			//	* [u8] seq
			//	* [bool] success
			//	* [u32] channel
			//	* [string] response name
			//	
			mux_.post(task_
				, IPT_EVENT_PUSH_CHANNEL_CLOSED
				, cyng::tuple_factory(frame.at(1)
					, tp_res_close_push_channel_policy::is_success(res)
					, frame.at(3)
					, tp_res_close_push_channel_policy::get_response_name(res)));
		}

		void bus::ipt_res_transfer_push_data(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx.attach(cyng::generate_invoke("log.msg.trace", "ipt.res.transfer.pushdata", frame));
		}

		void bus::ipt_req_transmit_data(cyng::context& ctx)
		{
			//	[0b5d8da4-ce8d-4c4f-bb02-9a9f173391d4,1B1B1B1B010101017681063...2007101633789000000001B1B1B1B1A034843]
			const cyng::vector_t frame = ctx.get_frame();

			auto bp = cyng::object_cast<cyng::buffer_t>(frame.at(1));
			BOOST_ASSERT_MSG(bp != nullptr, "no data");
			if (state_ != STATE_CONNECTED_) {
				ctx.attach(cyng::generate_invoke("log.msg.warning", "ipt.req.transmit.data - wrong state", get_state()));
			}
			else {
				ctx.attach(cyng::generate_invoke("log.msg.trace", "ipt.req.transmit.data", bp->size()));
			}

			mux_.post(task_
				, IPT_EVENT_INCOMING_DATA
				, cyng::tuple_factory(frame.at(1)));
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
			ctx.attach(cyng::generate_invoke("log.msg.debug", "ipt.req.open.connection", frame, get_state()));

			switch (state_)
			{
			case STATE_AUTHORIZED_:
				//
				//	[u8] seq
				//	[string] number
				//
				mux_.post(task_
					, IPT_EVENT_INCOMING_CALL
					, cyng::tuple_factory(frame.at(1), frame.at(2)));
				break;
			case STATE_CONNECTED_:
				//	busy
				ctx	.attach(cyng::generate_invoke("res.open.connection", frame.at(1), static_cast<std::uint8_t>(ipt::tp_res_open_connection_policy::BUSY)))
					.attach(cyng::generate_invoke("stream.flush"));
				break;
			default:
				ctx	.attach(cyng::generate_invoke("res.open.connection", frame.at(1), static_cast<std::uint8_t>(ipt::tp_res_open_connection_policy::DIALUP_FAILED)))
					.attach(cyng::generate_invoke("stream.flush"));
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

			const auto r = tp_res_open_connection_policy::is_success(std::get<2>(tpl));
			const auto msg = tp_res_open_connection_policy::get_response_name(std::get<2>(tpl));

			ctx.attach(cyng::generate_invoke("log.msg.debug", "ipt.res.open.connection", msg));

			auto pos = task_db_.find(std::get<1>(tpl));
			if (pos != task_db_.end())
			{
				const auto tsk = pos->second;
				ctx.attach(cyng::generate_invoke("log.msg.trace", "stop task", tsk));
				mux_.post(tsk, 0, cyng::tuple_factory(r));

				//
				//	remove entry
				//
				task_db_.erase(pos);
			}
			else
			{
				ctx.attach(cyng::generate_invoke("log.msg.warning", "empty sequence/task relation", std::get<1>(tpl)));
			}

			switch (state_)
			{
			case STATE_WAIT_FOR_OPEN_RESPONSE_:
				//
				//	update task state
				//
				state_ = STATE_CONNECTED_;

				// 
				//	* [u8] seq
				//	* [bool] success flag
				//	
				mux_.post(task_
					, IPT_EVENT_CONNECTION_OPEN
					, cyng::tuple_factory(frame.at(1), r));

				break;
			case STATE_CONNECTED_:
				ctx.attach(cyng::generate_invoke("log.msg.warning", "ipt.res.open.connection already connected", frame));
				break;
			default:
				ctx.attach(cyng::generate_invoke("log.msg.warning", "ipt.res.open.connection in wrong state", get_state()));
				break;
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

			switch (state_)
			{
			case STATE_AUTHORIZED_:
				ctx.attach(cyng::generate_invoke("log.msg.warning", "received ipt.req.close.connection - not connected", frame));
				break;
			case STATE_CONNECTED_:
				ctx.attach(cyng::generate_invoke("log.msg.debug", "received ipt.req.close.connection", frame));

				//
				//	update connection state
				//	no action from client required
				//
				state_ = STATE_AUTHORIZED_;

				{
					const auto seq = cyng::value_cast<sequence_type>(frame.at(1), 0u);

					//
					//	search for origin task
					//
					const auto pos = task_db_.find(seq);
					const auto origin = (pos != task_db_.end())
						? pos->second
						: cyng::async::NO_TASK
						;

					//	
					//	* [u8] seq
					//
					mux_.post(task_
						, IPT_EVENT_CONNECTION_CLOSED
						, cyng::tuple_factory(frame.at(1)
							, true		//	request
							, origin));
				}
				break;
			default:
				ctx.attach(cyng::generate_invoke("log.msg.error", "received ipt.req.close.connection - invalid state", get_state()));
				break;
			}

			//
			//	accept request in every case - send response
			//
			ctx	.attach(cyng::generate_invoke("res.close.connection", frame.at(1), static_cast<std::uint8_t>(ipt::tp_res_close_connection_policy::CONNECTION_CLEARING_SUCCEEDED)))
				.attach(cyng::generate_invoke("stream.flush"));

		}

		void bus::ipt_res_close_connection(cyng::context& ctx)
		{
			//
			//	* [uuid] session tag
			//	* [u8] seq
			//	* [u8] response
			//
			const cyng::vector_t frame = ctx.get_frame();
			ctx.attach(cyng::generate_invoke("log.msg.debug", "ipt.res.close.connection", frame));

			if (STATE_WAIT_FOR_CLOSE_RESPONSE_ == state_) {

				//
				//	update bus state
				//
				state_ = STATE_AUTHORIZED_;

				//
				//	get sequence
				//
				const auto seq = cyng::value_cast<sequence_type>(frame.at(1), 0u);

				//
				//	search for origin task
				//
				const auto pos = task_db_.find(seq);
				const auto origin = (pos != task_db_.end())
					? pos->second
					: cyng::async::NO_TASK
					;

				mux_.post(task_
					, IPT_EVENT_CONNECTION_CLOSED
					, cyng::tuple_factory(frame.at(1)
					, false		//	response
					, origin));
			}
			else {
				//	error
				CYNG_LOG_WARNING(logger_, "received a connection close response but didn't wait for one: " << get_state());
			}
		}


		void bus::ipt_req_protocol_version(cyng::context& ctx)
		{
			BOOST_ASSERT(vm_.tag() == ctx.tag());
			const cyng::vector_t frame = ctx.get_frame();
			ctx	.attach(cyng::generate_invoke("log.msg.debug", "ipt.req.protocol.version", frame))
				.attach(cyng::generate_invoke("res.protocol.version", frame.at(1), static_cast<std::uint8_t>(1)))
				.attach(cyng::generate_invoke("stream.flush"));
		}

		void bus::ipt_req_software_version(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx	.attach(cyng::generate_invoke("log.msg.debug", "ipt.req.software.version", frame))
				.attach(cyng::generate_invoke("res.software.version", frame.at(1), NODE_VERSION))
				.attach(cyng::generate_invoke("stream.flush"));
		}

		void bus::ipt_req_device_id(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx	.attach(cyng::generate_invoke("log.msg.debug", "ipt.req.device.id", frame))
				.attach(cyng::generate_invoke("res.device.id", frame.at(1), model_))
				.attach(cyng::generate_invoke("stream.flush"));
		}

		void bus::ipt_req_net_stat(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx	.attach(cyng::generate_invoke("log.msg.debug", "ipt.req.net.stat", frame))
				.attach(cyng::generate_invoke("res.unknown.command", frame.at(1), static_cast<std::uint16_t>(code::APP_REQ_NETWORK_STATUS)))
				.attach(cyng::generate_invoke("stream.flush"));
		}

		void bus::ipt_req_ip_statistics(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx	.attach(cyng::generate_invoke("log.msg.debug", "ipt.req.ip.statistics", frame))
				.attach(cyng::generate_invoke("res.unknown.command", frame.at(1), static_cast<std::uint16_t>(code::APP_REQ_IP_STATISTICS)))
				.attach(cyng::generate_invoke("stream.flush"));
		}

		void bus::ipt_req_dev_auth(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx	.attach(cyng::generate_invoke("log.msg.debug", "ipt.req.device.auth", frame))
				.attach(cyng::generate_invoke("res.unknown.command", frame.at(1), static_cast<std::uint16_t>(code::APP_REQ_DEVICE_AUTHENTIFICATION)))
				.attach(cyng::generate_invoke("stream.flush"));
		}

		void bus::ipt_req_dev_time(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			ctx	.attach(cyng::generate_invoke("log.msg.debug", "ipt.req.device.time", frame))
				.attach(cyng::generate_invoke("res.device.time", frame.at(1)))
				.attach(cyng::generate_invoke("stream.flush"));
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
			ctx.attach(cyng::generate_invoke("log.msg.debug", "ipt.req.transfer.pushdata"
				, std::get<2>(tpl)
				, std::get<3>(tpl)
				, std::get<6>(tpl).size()
				, "bytes"));


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
			mux_.post(task_
				, IPT_EVENT_PUSH_DATA_RECEIVED
				, cyng::tuple_factory(frame.at(1), frame.at(2), frame.at(3), frame.at(6)));
		}

		void bus::req_connection_open(std::string const& number, std::chrono::seconds d)
		{
			if (state_ == STATE_AUTHORIZED_) {
				state_ = STATE_WAIT_FOR_OPEN_RESPONSE_;
			}
			else {
				CYNG_LOG_WARNING(logger_, vm_.tag() << " try to open connection but is not authorized: " << get_state());
			}

			//
			//	start monitor tasks with 1 retry
			//
			cyng::async::start_task_sync<open_connection>(mux_, logger_, vm_, number, d, retries_);
		}

		void bus::req_connection_close(std::chrono::seconds d)
		{
			if (state_ == STATE_CONNECTED_) {
				state_ = STATE_WAIT_FOR_CLOSE_RESPONSE_;
			}
			else {
				CYNG_LOG_WARNING(logger_, vm_.tag() << " try to close connection but is not connected: " << get_state());
			}

			//
			//	start monitor tasks
			//
			cyng::async::start_task_sync<close_connection>(mux_, logger_, vm_, d);
		}

		void bus::res_connection_open(sequence_type seq, bool accept)
		{
			const response_type res = (accept)
				? ipt::tp_res_open_connection_policy::DIALUP_SUCCESS
				: ipt::tp_res_open_connection_policy::DIALUP_FAILED
				;

			if (accept && (state_ == STATE_AUTHORIZED_)) {
				//
				//	update task state
				//
				state_ = STATE_CONNECTED_;
			}

			vm_	.async_run(cyng::generate_invoke("res.open.connection", seq, res))
				.async_run(cyng::generate_invoke("stream.flush"));

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
			CYNG_LOG_INFO(logger_, "bus.store.relation " << +std::get<0>(tpl) << " => #" << std::get<1>(tpl));

			//
			//	store seq => task relation
			//
			task_db_.emplace(std::get<0>(tpl), std::get<1>(tpl));
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
			default:
				break;
			}
			return "ERROR";
		}


		bus::shared_type bus_factory(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, boost::uuids::uuid tag
			, scramble_key const& sk
			, std::size_t tsk
			, std::string const& model
			, std::size_t retries)
		{
			return std::make_shared<bus>(mux, logger, tag, sk, tsk, model, retries);
		}

		std::uint64_t build_line(std::uint32_t channel, std::uint32_t source)
		{
			//
			//	create the line ID by combining source and channel into one 64 bit integer
			//
			return (((std::uint64_t)channel) << 32) | ((std::uint64_t)source);
		}

	}
}
