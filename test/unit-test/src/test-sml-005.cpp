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
			using msg_0 = std::tuple<std::uint16_t, std::string>;
			using msg_1 = std::tuple<>;
			using msg_2 = std::tuple<sequence_type, std::string>;
			using msg_3 = std::tuple<sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t>;
			using msg_4 = std::tuple<sequence_type, bool, std::uint32_t>;
			using msg_5 = std::tuple<cyng::buffer_t>;
			using msg_6 = std::tuple<sequence_type, bool, std::string>;
			using msg_7 = std::tuple<sequence_type>;
			using msg_8 = std::tuple<sequence_type, response_type, std::uint32_t, std::uint32_t, std::uint16_t, std::size_t>;
			using msg_9 = std::tuple<sequence_type, response_type, std::uint32_t>;
			using signatures_t = std::tuple<msg_0, msg_1, msg_2, msg_3, msg_4, msg_5, msg_6, msg_7, msg_8, msg_9>;

		public:
			client(cyng::async::base_task* btp, cyng::logging::log_ptr logger)
				: base_(*btp)
				, bus_(bus_factory(btp->mux_, logger, boost::uuids::random_generator()(), scramble_key::default_scramble_key_, btp->get_id(), "ipt:test"))
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
			* @brief slot [0]
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
			* reconnect
			*/
			cyng::continuation process()
			{
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			* @brief slot [2]
			*
			* incoming call
			*/
			cyng::continuation process(sequence_type, std::string const& number)
			{
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			* @brief slot [3]
			*
			* push data
			*/
			cyng::continuation process(sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t const&)
			{
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			* @brief slot [4]
			*
			* register target response
			*/
			cyng::continuation process(sequence_type, bool, std::uint32_t)
			{
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			* @brief slot [5]
			*
			* incoming data
			*/
			cyng::continuation process(cyng::buffer_t const&)
			{
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			* @brief slot [6]
			*
			* push target deregistered
			*/
			cyng::continuation process(sequence_type, bool, std::string const&)
			{
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			* @brief slot [7]
			*
			* open connection closed
			*/
			cyng::continuation process(sequence_type)
			{
				return cyng::continuation::TASK_CONTINUE;
			}

			/**
			 * @brief slot [8]
			 *
			 * open push channel response
			 * @param type consumer type
			 * @param tid task id
			 */
			cyng::continuation process(sequence_type seq, response_type res, std::uint32_t channel, std::uint32_t source, std::uint16_t status, std::size_t count)
			{	
				const auto r = make_tp_res_open_push_channel(seq, res);
				CYNG_LOG_INFO(logger_, "ipt.res.open.push.channel: "
					<< channel
					<< ':'
					<< source
					<< ':'
					<< r.get_response_name());

				//
				//	send data
				//
				//std::ifstream file("C:\\projects\\workplace\\node\\Debug\\sml\\smf-a541f58-cea267c-2018080T21645-SML-water@solostec.bin", std::ios::binary | std::ios::app);
				//std::ifstream file("C:\\projects\\workplace\\node\\Debug\\sml\\smf-d918cce4-65d7aa73-2018080T121715-SML-water@solostec.bin", std::ios::binary | std::ios::app);
				//std::ifstream file("C:\\projects\\workplace\\node\\Debug\\sml\\smf-b3c1e378-1957aa1e-2018080T171813-IEC-LZQJ.bin", std::ios::binary | std::ios::app);
				std::ifstream file("C:\\projects\\workplace\\node\\Debug\\sml\\smf-f5e0e04f-8bd1a8b5-2018080T201923-IEC-LZQJ.bin", std::ios::binary | std::ios::app);
				
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

				return (r.is_success())
					? cyng::continuation::TASK_CONTINUE
					: cyng::continuation::TASK_STOP
					;
			}

			/**
			 * @brief slot [9]
			 *
			 * register consumer.
			 * open push channel response
			 * @param seq ipt sequence
			 * @param res channel open response
			 * @param channel channel id
			 */
			cyng::continuation process(sequence_type seq
				, response_type res
				, std::uint32_t channel)
			{
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
