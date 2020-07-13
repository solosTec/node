/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "connect.h"
#include "push.h"
#include "../cache.h"
#include "../storage.h"
#include "../bridge.h"
#include "../segw.h"

#include <smf/serial/baudrate.h>
#include <smf/ipt/response.hpp>
#include <smf/ipt/generator.h>
#include <smf/sml/protocol/serializer.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/event.h>
#include <smf/sml/srv_id_io.h>

#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/vm/domain/task_domain.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/buffer_cast.h>
#include <cyng/async/task/task_builder.hpp>

//#ifdef SMF_IO_DEBUG
#include <cyng/io/hex_dump.hpp>
//#endif

namespace node
{
	namespace ipt
	{
		connect::connect(cyng::async::base_task* btp
			, cyng::logging::log_ptr logger
			, std::string account
			, std::string pwd
			, std::string host
			, std::string service
			, boost::uuids::uuid tag)
		: bus(logger
				, btp->mux_
				, tag	
				, "SMF-GW:ETH"
				, 1u)
			, base_(*btp)
			, logger_(logger)
			, rec_(host, service, account, pwd, scramble_key::default_scramble_key_, false, 2)
		{
			CYNG_LOG_INFO(logger_, "initialize task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< ">");

			//
			//	register the task manager on this VM
			//
			cyng::register_task(btp->mux_, vm_);

			vm_.register_function("update.status.ip", 2, [&](cyng::context& ctx) {

				//	 [192.168.1.200:59536,192.168.1.100:26862]
				auto const frame = ctx.get_frame();
				CYNG_LOG_INFO(logger_, ctx.get_name()
					<< " - "
					<< cyng::io::to_str(frame));

				auto const tpl = cyng::tuple_cast<
					boost::asio::ip::tcp::endpoint,		//	[0] remote
					boost::asio::ip::tcp::endpoint		//	[1] local
				>(frame);

			});

			vm_.register_function("bus.reconfigure", 1, std::bind(&connect::reconfigure, this, std::placeholders::_1));

			//
			//	statistics
			//
			vm_.async_run(cyng::generate_invoke("log.msg.debug", cyng::invoke("lib.size"), " callbacks registered"));

		}

		cyng::continuation connect::run()
		{

			if (is_online())
			{
				//
				//	re/start monitor
				//
				base_.suspend(std::chrono::seconds(5));
			}
			else
			{
				//
				//	login request
				//
				req_login(rec_);
			}

			return cyng::continuation::TASK_CONTINUE;
		}

		void connect::stop(bool shutdown)
		{
			//
			//	call base class
			//
			bus::stop();
			while (!vm_.is_halted()) {
				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> continue gracefull shutdown");
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> is stopped");
		}

		void connect::reconfigure(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_WARNING(logger_, "bus.reconfigure " << cyng::io::to_str(frame));
		}

		//	slot [0] 0x4001/0x4002: response login
		void connect::on_login_response(std::uint16_t watchdog, std::string redirect)
		{

			//
			//	update IP address
			//
			vm_.async_run(cyng::generate_invoke("update.status.ip", cyng::invoke("ip.tcp.socket.ep.local"), cyng::invoke("ip.tcp.socket.ep.remote")));
		}

		//	slot [1] - connection lost / reconnect
		void connect::on_logout()
		{
			CYNG_LOG_WARNING(logger_, "LOGOUT");
		}

		//	slot [2] - 0x4005: push target registered response
		void connect::on_res_register_target(sequence_type seq
			, bool success
			, std::uint32_t channel
			, std::string target)
		{	//	no implementation
			BOOST_ASSERT_MSG(false, "[register target response] not implemented");
		}

		//	@brief slot [3] - 0x4006: push target deregistered response
		void connect::on_res_deregister_target(sequence_type, bool, std::string const&)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "push target deregistered response not implemented");
		}

		//	slot [4] - 0x1000: push channel open response
		void connect::on_res_open_push_channel(sequence_type seq
			, bool success
			, std::uint32_t channel
			, std::uint32_t source
			, std::uint16_t status
			, std::size_t count)
		{	//	use implementation in base class
		}

		//	slot [5] - 0x1001: push channel close response
		void connect::on_res_close_push_channel(sequence_type seq
			, bool success
			, std::uint32_t channel)
		{
			CYNG_LOG_INFO(logger_, "push channel "
				<< channel
				<< " close response received");
		}

		//	slot [6] 0x9003: connection open request 
		bool connect::on_req_open_connection(sequence_type seq, std::string const& number)
		{
			CYNG_LOG_TRACE(logger_, "incoming call " << +seq << ':' << number);

			BOOST_ASSERT(is_online());

			//
			//	accept incoming calls
			//
			return true;
		}

		//	slot [7] - 0x1003: connection open response
		cyng::buffer_t connect::on_res_open_connection(sequence_type seq, bool success)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "connection open response not implemented");
			return cyng::buffer_t();	//	no data
		}

		//	slot [8] - 0x9004/0x1004: connection close request/response
		void connect::on_req_close_connection(sequence_type seq)
		{	
			CYNG_LOG_INFO(logger_, "connection closed");
		}
		void connect::on_res_close_connection(sequence_type)
		{	//	no implementation
			CYNG_LOG_WARNING(logger_, "connection close response not implemented");
		}

		//	slot [9] - 0x9002: push data transfer request
		void connect::on_req_transfer_push_data(sequence_type seq
			, std::uint32_t channel
			, std::uint32_t source
			, cyng::buffer_t const& data)
		{
			//CYNG_LOG_INFO(logger_, "task #"
			//	<< base_.get_id()
			//	<< " <"
			//	<< base_.get_class_name()
			//	<< "> "
			//	<< config_.get().account_
			//	<< " received "
			//	<< data.size()
			//	<< " bytes push data from "
			//	<< channel
			//	<< '.'
			//	<< source);
		}

		//	slot [10] - transmit data (if connected)
		cyng::buffer_t connect::on_transmit_data(cyng::buffer_t const& data)
		{
			CYNG_LOG_TRACE(logger_, "incoming IPT/SML data " << data.size() << " bytes");
			if (data.size() == 6
				&& data.at(0) == 'h'
				&& data.at(1) == 'e'
				&& data.at(2) == 'l'
				&& data.at(3) == 'l'
				&& data.at(4) == 'o'
				&& data.at(5) == '!')
			{
				CYNG_LOG_DEBUG(logger_, "answer with hey!");
				return cyng::make_buffer({ 'h', 'e', 'y', '!' });
			}

#ifdef _DEBUG
			std::stringstream ss;
			cyng::io::hex_dump hd;
			hd(ss, data.begin(), data.end());
			CYNG_LOG_DEBUG(logger_, ss.str());
#endif
			return cyng::buffer_t();
		}

	}
}
