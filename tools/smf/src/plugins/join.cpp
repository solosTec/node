/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "join.h"
#include "../cli.h"
#include "tasks/cluster.h"
#include <smf/cluster/generator.h>
#include <cyng/async/task/task_builder.hpp>

#include <cyng/vm/generator.h>
#include <cyng/io/serializer.h>

#include <boost/algorithm/string.hpp>

namespace node
{
	join::join(cli* cp
		, cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, cluster_config_t const& cfg)
	: cli_(*cp)
		, mux_(mux)
		, logger_(logger)
		, tag_(tag)
		, config_(cfg)
		, task_(cyng::async::NO_TASK)
	{
		cli_.vm_.register_function("cluster", 1, std::bind(&join::cmd, this, std::placeholders::_1));

	}

	join::~join()
	{}

	void join::stop()
	{
		if (task_ != cyng::async::NO_TASK) {
			mux_.stop(task_);
		}
	}

	void join::cmd(cyng::context& ctx)
	{


		auto const frame = ctx.get_frame();
		auto const cmd = cyng::value_cast<std::string>(frame.at(0), "list");
		if (boost::algorithm::equals(cmd, "help")) {

			cli_.out_
				<< "help\tprint this page" << std::endl
				<< "join\t\tjoin cluster" << std::endl
				<< "stop\t\tleave cluster" << std::endl
				<< "show\t\tshow  configuration" << std::endl
				;
		}
		else if (boost::algorithm::equals(cmd, "join")) {
			cli_.out_
				<< "join: "
				<< cmd
				<< std::endl;

			connect();

		}
		else if (boost::algorithm::equals(cmd, "stop")) {
			if (task_ != cyng::async::NO_TASK) {
				cli_.out_
					<< "stop #"
					<< task_
					<< std::endl;

				mux_.stop(task_);
				task_ = cyng::async::NO_TASK;

			}
			else {
				cli_.out_
					<< "no cluster task is running!"
					<< std::endl;

			}
		}
		else if (boost::algorithm::equals(cmd, "show")) {
			cli_.out_
				<< std::endl;
		}
		else {
			cli_.out_
				<< "JOIN - unknown command: "
				<< cmd
				<< std::endl;
		}

	}

	void join::connect()
	{

		auto const r = cyng::async::start_task_delayed<cluster>(mux_
			, std::chrono::seconds(1)
			, logger_
			, tag_
			, config_);

		if (r.second) {

			task_ = r.first;
			CYNG_LOG_INFO(logger_, "cluster task# "
				<< task_
				<< " started");
		}
		else {
			CYNG_LOG_WARNING(logger_, "starting cluster task failed");
		}

	}

}