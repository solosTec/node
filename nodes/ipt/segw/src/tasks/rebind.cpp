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
		//boost::system::error_code ec;
		//acceptor_.close(ec);
		//if (ec) {
		//	//	error
		//	CYNG_LOG_WARNING(logger_, "task #"
		//		<< base_.get_id()
		//		<< " <"
		//		<< base_.get_class_name()
		//		<< "> NMS listener cannot be closed: "
		//		<< ec.message());
		//}
		//
		//acceptor_.open(ep.protocol(), ec);
		//if (ec) {
		//	//	error
		//	CYNG_LOG_WARNING(logger_, "task #"
		//		<< base_.get_id()
		//		<< " <"
		//		<< base_.get_class_name()
		//		<< "> NMS listener cannot open: "
		//		<< ec.message());
		//}

		//acceptor_.bind(ep, ec);
		//if (ec) { 
		//	//	error
		//	CYNG_LOG_WARNING(logger_, "task #"
		//		<< base_.get_id()
		//		<< " <"
		//		<< base_.get_class_name()
		//		<< "> NMS listener cannot bind new endpoint"
		//		<< ep
		//		<< ": "
		//		<< ec.message());
		//}
		//acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
		//if (ec) {
		//	//	error
		//	CYNG_LOG_WARNING(logger_, "task #"
		//		<< base_.get_id()
		//		<< " <"
		//		<< base_.get_class_name()
		//		<< "> NMS listener cannot bind new endpoint"
		//		<< ep
		//		<< ": "
		//		<< ec.message());
		//	return cyng::continuation::TASK_CONTINUE;
		//}

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

