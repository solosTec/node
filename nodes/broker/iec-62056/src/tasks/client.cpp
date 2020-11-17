/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <tasks/client.h>
#include <smf/cluster/generator.h>

#include <cyng/vm/controller.h>
#include <cyng/io/io_bytes.hpp>
#include <cyng/io/hex_dump.hpp>
 #include <cyng/vm/domain/log_domain.h>

namespace node
{
	client::client(cyng::async::base_task* btp
		, cyng::controller& cluster
		, cyng::controller& vm
		, cyng::logging::log_ptr logger
		, cyng::store::db& db
		, boost::asio::ip::tcp::endpoint ep
		, std::chrono::seconds monitor
		, std::string const& meter
		, bool client_login
		, bool verbose)
	: base_(*btp)
		, cluster_(cluster)
		, vm_(vm)
		, logger_(logger)
		, cache_(db)
		, ep_(ep)
		, monitor_(monitor)
		, meter_(meter)
		, socket_(base_.mux_.get_io_service())
		, parser_([this](cyng::vector_t&& prg) -> void {

			//CYNG_LOG_TRACE(logger_, "task #"
			//	<< base_.get_id()
			//	<< " <"
			//	<< base_.get_class_name()
			//	<< "> IEC data: "
			//	<< cyng::io::to_type(prg));

			vm_.async_run(std::move(prg));

		}, verbose)
		, buffer_read_()
		, buffer_write_()
		, client_login_(client_login)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< ep_);

		reset_write_buffer();

		//
		//	register logger domain
		//
		cyng::register_logger(logger_, vm_);
		vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

		vm_.register_function("iec.data.start", 1, std::bind(&client::data_start, this, std::placeholders::_1));
		vm_.register_function("iec.data.line", 6, std::bind(&client::data_line, this, std::placeholders::_1));
		vm_.register_function("iec.data.bcc", 1, std::bind(&client::data_bcc, this, std::placeholders::_1));
		vm_.register_function("iec.data.eof", 2, std::bind(&client::data_eof, this, std::placeholders::_1));

	}

	cyng::continuation client::run()
	{	
		//
		//	start/restart timer
		//
		base_.suspend(monitor_);

		if (!socket_.is_open()) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> open connection to "
				<< ep_);

			try {

				boost::asio::ip::tcp::resolver resolver(base_.mux_.get_io_service());
				auto const endpoints = resolver.resolve(ep_);
				do_connect(endpoints);
			}
			catch (std::exception const& ex) {

				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> "
					<< ex.what());
			}
		}


		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation client::process()
	{
		return cyng::continuation::TASK_CONTINUE;
	}

	void client::stop(bool shutdown)
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> remove embedded vm "
			<< vm_.tag());

		cluster_.async_run(cyng::generate_invoke("vm.remove", vm_.tag()));

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped");
	}

	void client::do_connect(const boost::asio::ip::tcp::resolver::results_type& endpoints)
	{
		boost::asio::async_connect(socket_, endpoints,
			[this](boost::system::error_code ec, boost::asio::ip::tcp::endpoint ep)
			{
				if (!ec) {
					CYNG_LOG_INFO(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> connected to "
						<< ep);

					cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
						, "connected"
						, ep
						, cluster_.tag()));

					//
					//	send query
					//
					do_write();

					//
					//	start reading
					//
					do_read();
				}
				else {
					CYNG_LOG_WARNING(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> "
						<< ep_
						<< ' '
						<< ec.message());

					cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
						, "connect failed"
						, ep_
						, cluster_.tag()));

				}
			});
	}

	void client::do_read()
	{
		socket_.async_read_some(boost::asio::buffer(buffer_read_.data(), buffer_read_.size()),
			[this](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec) {

#ifdef _DEBUG
					cyng::io::hex_dump hd;
					std::stringstream ss;
					hd(ss, buffer_read_.begin(), buffer_read_.begin() + bytes_transferred);
					CYNG_LOG_TRACE(logger_, "\n" << ss.str());
#endif

					//cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
					//	, std::string(buffer_read_.begin(), buffer_read_.begin() + bytes_transferred)
					//	, socket_.remote_endpoint()
					//	, cluster_.tag()));

					//
					//	parse IEC data
					//
					parser_.read(buffer_read_.begin(), buffer_read_.begin() + bytes_transferred);

					//
					//	continue reading
					//
					do_read();

					CYNG_LOG_TRACE(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> received "
						<< cyng::bytes_to_str(bytes_transferred));
				}
				else
				{
					CYNG_LOG_WARNING(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> "
						<< ep_
						<< ' '
						<< ec.message());

					//
					//	socket should already be closed
					//
					socket_.close(ec);

					reset_write_buffer();

				}

			});

	}

	void client::do_write()
	{
		if (!buffer_write_.empty()) {
			boost::asio::async_write(socket_,
				boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
				[this](boost::system::error_code ec, std::size_t bytes_transferred)
				{
					if (!ec)
					{
						CYNG_LOG_TRACE(logger_, "task #"
							<< base_.get_id()
							<< " <"
							<< base_.get_class_name()
							<< "> send "
							<< cyng::bytes_to_str(bytes_transferred));


						CYNG_LOG_TRACE(logger_, "task #"
							<< base_.get_id()
							<< " <"
							<< base_.get_class_name()
							<< "> send "
							<< cyng::io::to_hex(buffer_write_.front()));

						

						buffer_write_.pop_front();
						if (!buffer_write_.empty())
						{
							do_write();
						}
					}
					else
					{
						//
						//	no more data will be send
						//
						socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
					}
				});
		}
	}

	void client::reset_write_buffer()
	{
		buffer_write_.clear();
		std::string const hello("/?" + meter_ + "!\r\n");
		buffer_write_.emplace_back(cyng::buffer_t(hello.begin(), hello.end()));

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> query: "
			<< hello);

	}

	void client::data_start(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
			, "start reading"
			, ep_
			, cluster_.tag()));

	}

	void client::data_line(cyng::context& ctx)
	{
		//	[c8b5e3fb-ad83-47c8-ab21-357aec360af6,0000470700FF,0.000,A,,28]
		auto const frame = ctx.get_frame();
		CYNG_LOG_INFO(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		std::stringstream ss;
		ss
			<< cyng::io::to_str(frame.at(1))
			<< ": "
			<< cyng::io::to_str(frame.at(2))
			<< ' '
			<< cyng::io::to_str(frame.at(3))
			;
		cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
			, ss.str()
			, ep_
			, cluster_.tag()));
	}

	void client::data_bcc(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

	}

	void client::data_eof(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		cluster_.async_run(bus_insert_IEC_uplink(std::chrono::system_clock::now()
			, "stop reading"
			, ep_
			, cluster_.tag()));

		//
		//	close socket
		//
		boost::system::error_code ec;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket_.close(ec);

	}

}
