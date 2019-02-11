/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "push_ops.h"
#include <smf/sml/event.h>

#include <cyng/async/task/base_task.h>
#include <cyng/chrono.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/vm/generator.h>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

namespace node
{
	namespace ipt
	{
		push_ops::push_ops(cyng::async::base_task* btp
			, cyng::logging::log_ptr logger
			, node::sml::status& status_word
			, cyng::store::db& config_db
			, cyng::controller& vm
			, cyng::table::key_type const& key
			, boost::uuids::uuid tag)
		: base_(*btp)
			, logger_(logger)
			, status_word_(status_word)
			, config_db_(config_db)
			, vm_(vm)
			, key_(key)
			, tag_(tag)
			, state_(TASK_STATE_INITIAL_)
		{
			CYNG_LOG_INFO(logger_, "initialize task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< ">");
		}

		cyng::continuation push_ops::run()
		{

			switch (state_) {
			case TASK_STATE_INITIAL_:

				//
				//	update task state
				//
				state_ = TASK_STATE_RUNNING_;

				//
				//	update "push.ops" table with this task id
				//
				config_db_.modify("push.ops", key_, cyng::param_factory("task", base_.get_id()), tag_);

				//
				//	calculate next timepoint
				//
#ifdef _DEBUG
				push();
#endif
				set_tp();
				break;
			default:
				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> push data");
				push();
				set_tp();
				break;
			}


			//
			//	start interval timer
			//

			return cyng::continuation::TASK_CONTINUE;
		}

		void push_ops::stop()
		{
			//bus_->stop();
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> is stopped");
		}

		//	slot 0
		cyng::continuation push_ops::process(bool success
			, std::uint32_t channel
			, std::uint32_t source
			, std::uint16_t status
			, std::size_t count
			, std::string target)
		{
			if (success) {

				CYNG_LOG_TRACE(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> push channel "
					<< target
					<< " is open");

				//
				//	ToDo: push data
				//
				//
				for (auto const& entry : boost::make_iterator_range(boost::filesystem::directory_iterator("D:\\installations\\EBS\\reimport"), {}))
				{
					//const std::string file_name = "D:\\installations\\EBS\\debug\\smf--SML-201812T191303-pushStore49b04d0a-31a3a9c5.sml";
					const std::string file_name = entry.path().string();
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
						file.close();

						CYNG_LOG_WARNING(logger_, "task #"
							<< base_.get_id()
							<< " <"
							<< base_.get_class_name()
							<< "> remove "
							<< entry.path());
						boost::filesystem::remove(entry.path());

						config_db_.insert("op.log"
							, cyng::table::key_generator(0UL)
							, cyng::table::data_generator(std::chrono::system_clock::now()
								, static_cast<std::uint32_t>(900u)	//	reg period - 15 min
								, std::chrono::system_clock::now()	//	val time
								, static_cast<std::uint64_t>(status_word_.operator std::uint64_t())	//	status
								, node::sml::evt_push_succes()	//	event - push successful
								, cyng::make_buffer({ 0x81, 0x46, 0x00, 0x00, 0x02, 0xFF })
								, std::chrono::system_clock::now()	//	val time
								, cyng::make_buffer({ 0x02, 0xE6, 0x1E, 0x27, 0x66, 0x03, 0x15, 0x35, 0x03 })
								, target
								, static_cast<std::uint8_t>(1u))		//	push_nr
							, 1	//	generation
							, tag_);



						break;	//	one item at a time
					}
				}

				//
				//	close push channel
				//
				CYNG_LOG_TRACE(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> close push channel "
					<< channel
					<< ':'
					<< source);
				vm_.async_run(cyng::generate_invoke("req.close.push.channel", channel));

			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> open push channel "
					<< target
					<< " failed");

			}
			return cyng::continuation::TASK_CONTINUE;
		}

		void push_ops::set_tp()
		{
			config_db_.access([&](cyng::store::table const* tbl) {

				auto rec = tbl->lookup(this->key_);

				const std::chrono::seconds interval(cyng::value_cast<std::uint32_t>(rec["interval"], 900u));
				if (interval < std::chrono::minutes(1)) {

				}
				else if (interval < std::chrono::minutes(60)) {

					const std::chrono::minutes interval_minutes = std::chrono::duration_cast<std::chrono::minutes>(interval);

					std::tm tmp = cyng::chrono::make_utc_tm(std::chrono::system_clock::now());
					auto current_minute = cyng::chrono::minute(tmp);
					auto modulo_minutes = current_minute % interval_minutes.count();
					BOOST_ASSERT(current_minute >= modulo_minutes);

					//
					//	get next optimal time point to start
					//
					auto ntp = cyng::chrono::init_tp(cyng::chrono::year(tmp)
						, cyng::chrono::month(tmp)
						, cyng::chrono::day(tmp)
						, cyng::chrono::hour(tmp)
						, (current_minute - modulo_minutes)
						, 0.0) + interval_minutes;

					CYNG_LOG_INFO(logger_, "task #"
						<< base_.get_id()
						<< " interval: "
						<< cyng::to_str(interval)
						<< " - next data push at "
						<< cyng::to_str(ntp)
						<< " UTC");

					this->base_.suspend_until(ntp);
				}
				else if (interval < std::chrono::minutes(60 * 24)) {

				}
				else {

				}

			}	, cyng::store::read_access("push.ops"));
		}

		void push_ops::push()
		{
			if (status_word_.is_authorized()) {
				config_db_.access([&](cyng::store::table const* tbl) {

					auto rec = tbl->lookup(this->key_);
					const auto target(cyng::value_cast<std::string>(rec["target"], ""));

					CYNG_LOG_INFO(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> open channel "
						<< target);

					vm_
						.async_run(cyng::generate_invoke("req.open.push.channel", target, "", "", "", "", 0))
						.async_run(cyng::generate_invoke("bus.store.rel.channel.open", cyng::invoke("ipt.seq.push"), base_.get_id(), target))
						.async_run(cyng::generate_invoke("stream.flush", target))
						.async_run(cyng::generate_invoke("log.msg.info", "req.open.push.channel", cyng::invoke("ipt.seq.push")))
						;

				}, cyng::store::read_access("push.ops"));
			}
			else {

				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> is offline");

			}
		}
	}
}
