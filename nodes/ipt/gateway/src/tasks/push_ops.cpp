/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "push_ops.h"
#include <cyng/async/task/base_task.h>
#include <cyng/chrono.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/vm/generator.h>

namespace node
{
	namespace ipt
	{
		push_ops::push_ops(cyng::async::base_task* btp
			, cyng::logging::log_ptr logger
			, node::sml::status& status_word
			, cyng::store::db& config_db
			, node::ipt::bus::shared_type bus
			, cyng::table::key_type const& key
			, boost::uuids::uuid tag)
		: base_(*btp)
			, logger_(logger)
			, status_word_(status_word)
			, config_db_(config_db)
			, bus_(bus)
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
			bus_->stop();
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
				bus_->vm_.async_run(cyng::generate_invoke("req.close.push.channel", channel));

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

					bus_->vm_
						.async_run(cyng::generate_invoke("req.open.push.channel", target, "", "", "", "", 0))
						.async_run(cyng::generate_invoke("bus.store.rel.channel.open", cyng::invoke("ipt.push.seq"), base_.get_id(), target))
						.async_run(cyng::generate_invoke("stream.flush", target))
						.async_run(cyng::generate_invoke("log.msg.info", "req.open.push.channel", cyng::invoke("ipt.push.seq")))
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
