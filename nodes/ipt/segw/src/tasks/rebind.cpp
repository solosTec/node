/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/rebind.h>
#include <bridge.h>

namespace node
{
	rebind::rebind(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, boost::asio::ip::tcp::acceptor& s)
	: base_(*btp) 
		, logger_(logger)
		, acceptor_(s)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> at "
			<< s.local_endpoint());

	}

	cyng::continuation rebind::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
#endif

		//
		//	start/restart timer
		//
		//base_.suspend(interval_time_);

		return cyng::continuation::TASK_CONTINUE;
	}

	void rebind::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation rebind::process(boost::asio::ip::tcp::endpoint ep)
	{
		acceptor_ = boost::asio::ip::tcp::acceptor(acceptor_.get_executor(), ep);

		//
		//	alternative implementation
		//
		//	boost::system::error_code ec;
		//	acceptor_.close(ec);
		//	acceptor_.open(ep.protocol(), ec);
		//	acceptor_.bind(ep, ec);
		//	acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
		//

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> NMS listener has new endpoint"
			<< acceptor_.local_endpoint());

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

}

