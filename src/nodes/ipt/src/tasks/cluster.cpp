#include <tasks/cluster.h>
#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>

#include <iostream>

namespace smf {

	cluster::cluster(cyng::channel_weak wp
		, boost::uuids::uuid tag
		, cyng::logger logger
		, toggle cluster_cfg)
	: sigs_{ 
		std::bind(&cluster::connect, this),
		std::bind(&cluster::status_check, this, std::placeholders::_1),
		std::bind(&cluster::login, this, std::placeholders::_1),
		std::bind(&cluster::demo3, this, std::placeholders::_1),
		std::bind(&cluster::stop, this, std::placeholders::_1),
	}
		, channel_(wp)
		, tag_(tag)
		, logger_(logger)
		, bus_(logger, cluster_cfg, wp)
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("connect", 0);
			sp->set_channel_name("status_check", 1);
			sp->set_channel_name("login", 2);
			sp->set_channel_name("demo3", 3);
		}

		CYNG_LOG_INFO(logger_, "cluster task " << tag << " started");

	}

	cluster::~cluster()
	{
#ifdef _DEBUG_IPT
		std::cout << "cluster(~)" << std::endl;
#endif
	}


	void cluster::stop(cyng::eod)
	{
		CYNG_LOG_WARNING(logger_, "stop cluster task(" << tag_ << ")");
		bus_.stop();
	}

	void cluster::connect()
	{
		bus_.start();
	}

	void cluster::status_check(int n)
	{
		auto sp = channel_.lock();
		if (sp) {
			CYNG_LOG_TRACE(logger_, "status_check(" << tag_ << ", " << n << ")");
			sp->suspend(std::chrono::seconds(30), "status_check", cyng::make_tuple(n + 1));
		}
		else {
			CYNG_LOG_ERROR(logger_, "status_check(" << tag_ << ", " << n << ")");
		}
	}

	void cluster::login(bool success)
	{
		if (success) {
			CYNG_LOG_INFO(logger_, "start IP-T server");

			//
			//	ToDo: start IP-T server
			//
		}
	}

	void cluster::demo3(int n)
	{
#ifdef _DEBUG_IPT
		std::cout << "cluster::demo3(" << n << ")" << std::endl;
#endif
	}

}


