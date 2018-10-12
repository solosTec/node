/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include "test-sml-005.h"
#include <iostream>
#include <fstream>
#include <boost/test/unit_test.hpp>
#include <boost/uuid/random_generator.hpp>

#include <smf/sml/obis_io.h>
#include <smf/ipt/bus.h>
#include <smf/ipt/config.h>
#include <smf/ipt/generator.h>
#include <smf/ipt/response.hpp>

#include <cyng/async/task/task_builder.hpp>
#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/vm/generator.h>


namespace node 
{
	namespace ipt
	{
		class client
		{
		public:
			//	 [0] 0x4001/0x4002: response login
			using msg_00 = std::tuple<std::uint16_t, std::string>;
			using msg_01 = std::tuple<>;

			//	[2] 0x4005: push target registered response
			using msg_02 = std::tuple<sequence_type, bool, std::uint32_t, std::string>;

			//	[3] 0x4006: push target deregistered response
			using msg_03 = std::tuple<sequence_type, bool, std::string>;

			//	[4] 0x1000: push channel open response
			using msg_04 = std::tuple<sequence_type, bool, std::uint32_t, std::uint32_t, std::uint16_t, std::size_t>;

			//	[5] 0x1001: push channel close response
			using msg_05 = std::tuple<sequence_type, bool, std::uint32_t, std::string>;

			//	[6] 0x9003: connection open request 
			using msg_06 = std::tuple<sequence_type, std::string>;

			//	[7] 0x1003: connection open response
			using msg_07 = std::tuple<sequence_type, bool>;

			//	[8] 0x9004: connection close request
			using msg_08 = std::tuple<sequence_type, bool, std::size_t>;

			//	[9] 0x9002: push data transfer request
			using msg_09 = std::tuple<sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t>;

			//	[10] transmit data(if connected)
			using msg_10 = std::tuple<cyng::buffer_t>;

			using signatures_t = std::tuple<msg_00, msg_01, msg_02, msg_03, msg_04, msg_05, msg_06, msg_07, msg_08, msg_09, msg_10>;


		public:
			client(cyng::async::base_task* btp, cyng::logging::log_ptr logger)
				: base_(*btp)
				, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), scramble_key::default_scramble_key_, btp->get_id(), "ipt:test", 1u))
				, logger_(logger)
			{
				CYNG_LOG_INFO(logger_, "initialize task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< ">");
			}

			cyng::continuation run()
			{
				if (bus_->is_online())
				{
					//
					//	send watchdog response - without request
					//
					bus_->vm_.async_run(cyng::generate_invoke("res.watchdog", static_cast<std::uint8_t>(0)));
					bus_->vm_.async_run(cyng::generate_invoke("stream.flush"));
				}
				else
				{
					//
					//	reset parser and serializer
					//
					bus_->vm_.async_run(cyng::generate_invoke("ipt.reset.parser", scramble_key(scramble_key::default_scramble_key_)));
					bus_->vm_.async_run(cyng::generate_invoke("ipt.reset.serializer", scramble_key(scramble_key::default_scramble_key_)));

					//
					//	login request
					//
					master_record cfg("localhost", "26862", "User", "Pass", scramble_key::default_scramble_key_, false, 0);
					bus_->vm_.async_run(gen::ipt_req_login_public(cfg));
					//bus_->vm_.async_run(gen::ipt_req_login_scrambled(cfg));
				}
				return cyng::continuation::TASK_CONTINUE;
			}
			void stop()
			{
				bus_->stop();
				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> is stopped");
			}

			/**
			 * @brief slot [0] 0x4001/0x4002: response login
			 *
			 * sucessful network login
			 */
			cyng::continuation process(std::uint16_t watchdog, std::string redirect)
			{
				if (watchdog != 0)
				{
					CYNG_LOG_INFO(logger_, "start watchdog: " << watchdog << " minutes");
					base_.suspend(std::chrono::minutes(watchdog));
				}

				//
				//	open push channel
				//
				//bus_->vm_.async_run(cyng::generate_invoke("req.open.push.channel", "water@solostec", "", "", "", "", 0));
				bus_->vm_.async_run(cyng::generate_invoke("req.open.push.channel", "LZQJ", "", "", "", "", 0));
				
				bus_->vm_.async_run(cyng::generate_invoke("stream.flush"));
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			 * @brief slot [1]
			 *
			 * connection lost / reconnect
			 */
			cyng::continuation process()
			{
				CYNG_LOG_WARNING(logger_, "connection to ipt master lost");
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			 * @brief slot [2] 0x4005: push target registered response
			 *
			 * register target response
			 *
			 * @param seq ipt sequence
			 * @param success success flag
			 * @param channel channel id
			 * @param target target name (empty when if request not triggered by bus::req_register_push_target())
			 */
			cyng::continuation process(sequence_type, bool, std::uint32_t, std::string target)
			{
				CYNG_LOG_INFO(logger_, "cpush target registered response");
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			 * @brief slot [3] 0x4006: push target deregistered response
			 *
			 * deregister target response
			 */
			cyng::continuation process(sequence_type, bool, std::string const&)
			{
				CYNG_LOG_INFO(logger_, "cpush target deregistered response");
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			 * @brief slot [4] 0x1000: push channel open response
			 *
			 * open push channel response
			 * @param seq ipt sequence
			 * @param success success flag
			 * @param channel channel id
			 * @param source source id
			 * @param status channel status
			 * @param count number of targets reached
			 */
			cyng::continuation process(sequence_type seq, bool success, std::uint32_t channel, std::uint32_t source, std::uint16_t status, std::size_t count)
			{
				CYNG_LOG_INFO(logger_, "ipt.res.open.push.channel: "
					<< channel
					<< ':'
					<< source
					<< ':'
					<< (success ? " success" : " failed"));

				if (success) {

					const std::string file_name = "C:\\projects\\workplace\\node\\Debug\\sml\\smf-f5e0e04f-8bd1a8b5-2018080T201923-IEC-LZQJ.bin";
					CYNG_LOG_INFO(logger_, "push data of file " << file_name);

					//
					//	send data
					//
					//std::ifstream file("C:\\projects\\workplace\\node\\Debug\\sml\\smf-a541f58-cea267c-2018080T21645-SML-water@solostec.bin", std::ios::binary | std::ios::app);
					//std::ifstream file("C:\\projects\\workplace\\node\\Debug\\sml\\smf-d918cce4-65d7aa73-2018080T121715-SML-water@solostec.bin", std::ios::binary | std::ios::app);
					//std::ifstream file("C:\\projects\\workplace\\node\\Debug\\sml\\smf-b3c1e378-1957aa1e-2018080T171813-IEC-LZQJ.bin", std::ios::binary | std::ios::app);
					std::ifstream file(file_name, std::ios::binary | std::ios::app);

					if (file.is_open())
					{
						//	dont skip whitepsaces
						file >> std::noskipws;
						cyng::buffer_t data;
						data.insert(data.begin(), std::istream_iterator<char>(file), std::istream_iterator<char>());

						bus_->vm_.async_run(cyng::generate_invoke("req.transfer.push.data"
							, channel
							, source
							, std::uint8_t(0xC1)	//	status
							, std::uint8_t(0)	//	block
							, data));
						bus_->vm_.async_run(cyng::generate_invoke("stream.flush"));
					}

					//
					//	close channel
					//
					bus_->vm_.async_run(cyng::generate_invoke("req.close.push.channel", channel));
					bus_->vm_.async_run(cyng::generate_invoke("stream.flush"));

				}
				return (success)
					? cyng::continuation::TASK_CONTINUE
					: cyng::continuation::TASK_STOP
					;
			}

			/**
			 * @brief slot [5] 0x1001: push channel close response
			 *
			 * register consumer.
			 * open push channel response
			 * @param seq ipt sequence
			 * @param bool success flag
			 * @param channel channel id
			 * @param res response name
			 */
			cyng::continuation process(sequence_type seq
				, bool success
				, std::uint32_t channel
				, std::string res)
			{
				CYNG_LOG_INFO(logger_, "push channel close response");
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			 * @brief slot [6] 0x9003: connection open request 
			 *
			 * incoming call
			 */
			cyng::continuation process(sequence_type, std::string const& number)
			{
				CYNG_LOG_INFO(logger_, "connection open request ");
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			 * @brief slot [7] 0x1003: connection open response
			 *
			 * @param seq ipt sequence
			 * @param success true if connection open request was accepted
			 */
			cyng::continuation process(sequence_type seq, bool success)
			{
				CYNG_LOG_INFO(logger_, "connection open response ");
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			 * @brief slot [8] 0x9004/0x1004: connection close request/response
			 *
			 * open connection closed
			 */
			cyng::continuation process(sequence_type, bool, std::size_t)
			{
				CYNG_LOG_INFO(logger_, "connection close request/response");
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			 * @brief slot [9] 0x9002: push data transfer request
			 *
			 * push data
			 */
			cyng::continuation process(sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t const&)
			{
				CYNG_LOG_INFO(logger_, "push data transfer request");
				return cyng::continuation::TASK_CONTINUE;
			}


			/**
			 * @brief slot [10] transmit data (if connected)
			 *
			 * receive data
			 */
			cyng::continuation process(cyng::buffer_t const&)
			{
				CYNG_LOG_INFO(logger_, "transmit data");
				return cyng::continuation::TASK_CONTINUE;
			}

		private:
			cyng::async::base_task& base_;
			bus::shared_type bus_;
			cyng::logging::log_ptr logger_;
		};
	}

	bool test_sml_005()
	{
		cyng::async::mux task_manager;
		auto logger = cyng::logging::make_console_logger(task_manager.get_io_service(), "ipt:test");
		cyng::async::start_task_delayed<ipt::client>(task_manager, std::chrono::seconds(1), logger);

		std::this_thread::sleep_for(std::chrono::seconds(8));
		task_manager.stop();
		task_manager.get_io_service().stop();

		return true;
	}

}
