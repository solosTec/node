/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <tasks/broker.h>
#include <cache.h>
#include <smf/mbus/defs.h>
#include <smf/mbus/parser.h>

#include <cyng/vm/controller.h>
#include <cyng/io/serializer.h>
#include <cyng/io/io_bytes.hpp>
#include <cyng/compatibility.h>
#include <cyng/buffer_cast.h>

#include <iostream>

namespace node
{
	broker::broker(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, cache& cfg
		, std::string account
		, std::string pwd
		, std::string host
		, std::uint16_t port)
	: base_(*btp) 
		, logger_(logger)
		, cache_(cfg)
		, cfg_(cfg)
		, account_(account)
		, pwd_(pwd)
		, host_(host)
		, port_(port)
		, socket_(base_.mux_.get_io_service())
		, buffer_read_()
		, buffer_write_()
		, insert_connection_(cfg.get_db().get_insert_listener("_Readout", std::bind(&broker::sig_ins, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)))
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< account_
			<< ':'
			<< pwd_
			<< '@'
			<< host_
			<< ':'
			<< port_);

		//
		//	login message
		//
		reset_write_buffer();

	}

	void broker::sig_ins(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::table::data_type const& data
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> readout: "
			<< tbl->meta().get_name());

		BOOST_ASSERT_MSG(boost::algorithm::equals(tbl->meta().get_name(), "_Readout"), "wrong table");
		buffer_write_.clear();

		//
		//	get table data
		//
		auto const rec = cyng::table::record(tbl->meta_ptr(), key, data, gen);
		auto const srv_id = cyng::to_buffer(rec["serverID"]);
		auto const size = cyng::value_cast<std::uint8_t>(rec["size"], 0);
		auto const manufacturer = cyng::to_buffer(rec["manufacturer"]);
		auto const dev_id = cyng::value_cast<std::uint32_t>(rec["dev_id"], 0);
		auto const version = cyng::value_cast<std::uint8_t>(rec["version"], 0);
		auto const medium = cyng::value_cast<std::uint8_t>(rec["medium"], 0);
		auto const frame_type = cyng::value_cast<std::uint8_t>(rec["frame_type"], 0);
		auto const payload = cyng::to_buffer(rec["payload"]);

		//	length - 1 byte
		buffer_write_.emplace_back(cyng::buffer_t(1, size));	

		//	C-field - 1 byte (always 0x44)
		buffer_write_.emplace_back(cyng::buffer_t(1, mbus::CTRL_FIELD_SND_NR));

		//	manufacturer - 2 byte
		BOOST_ASSERT(manufacturer.size() == 2);
		buffer_write_.emplace_back(manufacturer);

		//	device id - 4 byte
		auto const dev = to_meter_id(dev_id);
		buffer_write_.emplace_back(dev);

		//	version  - 1 byte
		buffer_write_.emplace_back(cyng::buffer_t(1, version));

		//	medium - 1 byte
		buffer_write_.emplace_back(cyng::buffer_t(1, medium));

		//	frame type - 1 byte
		buffer_write_.emplace_back(cyng::buffer_t(1, frame_type));

		//	payload
		buffer_write_.emplace_back(payload);

		if (socket_.is_open()) {
			do_write();
		}
		else {
			reconnect();
		}

	}

	void broker::reset_write_buffer()
	{
		buffer_write_.clear();
		//buffer_write_.emplace_back(cyng::buffer_t(account_.begin(), account_.end()));
		//buffer_write_.emplace_back(cyng::buffer_t(1, ':'));
		//buffer_write_.emplace_back(cyng::buffer_t(pwd_.begin(), pwd_.end()));
		//buffer_write_.emplace_back(cyng::buffer_t(1, '\n'));
	}

	cyng::continuation broker::run()
	{
		if (/*!buffer_write_.empty() &&*/ !socket_.is_open()) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> open connection to "
				<< host_
				<< ':'
				<< port_);

			reconnect();
		}

		//
		//	start/restart timer
		//
		base_.suspend(std::chrono::seconds(12));

		return cyng::continuation::TASK_CONTINUE;
	}

	void broker::reconnect() {
		try {

			boost::asio::ip::tcp::resolver resolver(base_.mux_.get_io_service());
			auto const endpoints = resolver.resolve(host_, std::to_string(port_));
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

	void broker::stop(bool shutdown)
	{
		//
		//	remove as listener
		//
		insert_connection_.disconnect();

		//
		//  close socket
		//
		boost::system::error_code ec;
		socket_.close(ec);

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped: "
			<< ec.message());
	}

	cyng::continuation broker::process(cyng::buffer_t data, std::size_t msg_counter)
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> received msg #"
			<< msg_counter
			<< " with "
			<< cyng::bytes_to_str(data.size()));

		buffer_write_.push_back(data);

		//
		//  send to receiver
		//
		if (socket_.is_open()) {
			do_write();
		}

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	void broker::do_connect(const boost::asio::ip::tcp::resolver::results_type& endpoints)
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

					//
					//	send login
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
						<< host_
						<< ':'
						<< port_
						<< ' '
						<< ec.message());

				}
		});
	}

	void broker::do_read()
	{
		socket_.async_read_some(boost::asio::buffer(buffer_read_.data(), buffer_read_.size()),
			[this](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec && ec != boost::system::errc::broken_pipe) {

					//
					//	ToDo: forward to serial interface
					//

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
						<< host_
						<< ':'
						<< port_
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

	void broker::do_write()
	{
		if (buffer_write_.empty())	return;

		boost::asio::async_write(socket_,
			boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
			[this](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec && ec != boost::system::errc::broken_pipe)
				{
					buffer_write_.pop_front();
					if (!buffer_write_.empty())
					{
						do_write();
					}
				}
				else
				{
					CYNG_LOG_WARNING(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> "
						<< host_
						<< ':'
						<< port_
						<< ' '
						<< ec.message());

					//
					//	no more data will be send
					//
					buffer_write_.clear();
					socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
					socket_.close(ec);
				}
		});
	}

}

