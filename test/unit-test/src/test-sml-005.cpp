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
#include <cyng/io/serializer.h>


namespace node 
{
	namespace ipt
	{
		/**
		 * Login as IP-T client, opens a push channel and push content of a (hard coded) file.
		 * Change parameter in the test code to address different push targets and deliver the requested content.
		 */
		class client : public bus
		{
		public:
			using signatures_t = std::tuple<>;


		public:
			client(cyng::async::base_task* btp, cyng::logging::log_ptr logger)
				: bus(logger, btp->mux_, boost::uuids::random_generator()(), scramble_key::default_scramble_key_, "ipt:test", 1u)
				, base_(*btp)
				, logger_(logger)
			{
				CYNG_LOG_INFO(logger_, "initialize task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< ">");

				//
				//	request handler
				//
				vm_.register_function("bus.reconfigure", 1, std::bind(&client::reconfigure, this, std::placeholders::_1));

			}

			cyng::continuation run()
			{
				if (is_online())
				{
					//
					//	re/start monitor
					//
					//base_.suspend(config_.get().monitor_);
				}
				else
				{
					//
					//	reset parser and serializer
					//
					vm_.async_run({ cyng::generate_invoke("ipt.reset.parser", scramble_key(scramble_key::default_scramble_key_))
						, cyng::generate_invoke("ipt.reset.serializer", scramble_key(scramble_key::default_scramble_key_)) });

					//
					//	login request
					//
					master_record cfg("localhost", "26862", "User", "Pass", scramble_key::default_scramble_key_, false, 0);
					//master_record cfg("ebs.solostec.net", "26862", "SK-26-MUC", "hI12TOM3", scramble_key::default_scramble_key_, false, 0);	//	EBS test
					vm_.async_run(gen::ipt_req_login_public(cfg));
				}
				return cyng::continuation::TASK_CONTINUE;
			}
			void stop()
			{
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

			/**
			 * @brief slot [0] 0x4001/0x4002: response login
			 *
			 * sucessful network login
			 */
			virtual void on_login_response(std::uint16_t watchdog, std::string redirect) override
			{
				//
				//	open push channel
				//
				//bus_->vm_.async_run(cyng::generate_invoke("req.open.push.channel", "water@solostec", "", "", "", "", 0));
				//vm_.async_run(cyng::generate_invoke("req.open.push.channel", "LZQJ", "", "", "", "", 0));
				vm_.async_run(cyng::generate_invoke("req.open.push.channel", "data-store", "", "", "", "", 0));
				//vm_.async_run(cyng::generate_invoke("req.open.push.channel", "pushStore", "", "", "", "", 0));	//	EBS test
				vm_.async_run(cyng::generate_invoke("stream.flush"));
			}

			/**
			 * @brief slot [1]
			 *
			 * connection lost / reconnect
			 */
			virtual void on_logout() override
			{
				CYNG_LOG_WARNING(logger_, "connection to ipt master lost");
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
			virtual void on_res_register_target(sequence_type, bool, std::uint32_t, std::string target) override
			{
				CYNG_LOG_INFO(logger_, "cpush target registered response");
			}

			/**
			 * @brief slot [3] 0x4006: push target deregistered response
			 *
			 * deregister target response
			 */
			virtual void on_res_deregister_target(sequence_type, bool, std::string const&) override
			{
				CYNG_LOG_INFO(logger_, "push target deregistered response");
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
			virtual void on_res_open_push_channel(sequence_type seq, bool success, std::uint32_t channel, std::uint32_t source, std::uint16_t status, std::size_t count) override
			{
				CYNG_LOG_INFO(logger_, "ipt.res.open.push.channel: "
					<< channel
					<< ':'
					<< source
					<< ':'
					<< (success ? " success" : " failed"));

				if (success) {

					//const std::string file_name = "C:\\projects\\workplace\\node\\Debug\\sml\\smf-f5e0e04f-8bd1a8b5-2018080T201923-IEC-LZQJ.bin";
					const std::string file_name = "D:\\installations\\Tägerwilen\\debug\\smf-26ed3332-84767607-2018100T10108-SML-pushTBT.bin";
				
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

						vm_.async_run(cyng::generate_invoke("req.transfer.push.data"
							, channel
							, source
							, std::uint8_t(0xC1)	//	status
							, std::uint8_t(0)	//	block
							, data));
						vm_.async_run(cyng::generate_invoke("stream.flush"));
					}

					//
					//	close channel
					//
					vm_.async_run(cyng::generate_invoke("req.close.push.channel", channel));
					vm_.async_run(cyng::generate_invoke("stream.flush"));

				}
				//return (success)
				//	? cyng::continuation::TASK_CONTINUE
				//	: cyng::continuation::TASK_STOP
				//	;
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
			virtual void on_res_close_push_channel(sequence_type seq
				, bool success
				, std::uint32_t channel) override
			{
				CYNG_LOG_INFO(logger_, "push channel close response");
			}

			/**
			 * @brief slot [6] 0x9003: connection open request 
			 *
			 * incoming call
			 */
			virtual bool on_req_open_connection(sequence_type, std::string const& number) override
			{
				CYNG_LOG_INFO(logger_, "connection open request ");
				return false;
			}

			/**
			 * @brief slot [7] 0x1003: connection open response
			 *
			 * @param seq ipt sequence
			 * @param success true if connection open request was accepted
			 */
			virtual cyng::buffer_t on_res_open_connection(sequence_type seq, bool success) override
			{
				CYNG_LOG_INFO(logger_, "connection open response ");
				return cyng::buffer_t();
			}

			/**
			 * @brief slot [8] 0x9004/0x1004: connection close request/response
			 *
			 * open connection closed
			 */
			virtual void on_req_close_connection(sequence_type) override
			{}
			virtual void on_res_close_connection(sequence_type) override
			{}

			/**
			 * @brief slot [9] 0x9002: push data transfer request
			 *
			 * push data
			 */
			virtual void on_req_transfer_push_data(sequence_type, std::uint32_t, std::uint32_t, cyng::buffer_t const&) override
			{
				CYNG_LOG_INFO(logger_, "push data transfer request");
			}


			/**
			 * @brief slot [10] transmit data (if connected)
			 *
			 * receive data
			 */
			virtual cyng::buffer_t on_transmit_data(cyng::buffer_t const&) override
			{
				CYNG_LOG_INFO(logger_, "transmit data");
				return cyng::buffer_t();
			}

			void reconfigure(cyng::context& ctx)
			{
				const cyng::vector_t frame = ctx.get_frame();
				CYNG_LOG_WARNING(logger_, "bus.reconfigure " << cyng::io::to_str(frame));
				reconfigure_impl();
			}

			void reconfigure_impl()
			{
				//
				//	switch to other master
				//
				//"localhost", "26862", "User", "Pass"
				CYNG_LOG_ERROR(logger_, "network login failed - no other redundancy available");

				//
				//	trigger reconnect 
				//
				base_.suspend(std::chrono::seconds(10));
			}

		private:
			cyng::async::base_task& base_;
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
