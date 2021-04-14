/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <wmbus_session.h>

#include <smf/mbus/flag_id.h>
#include <smf/mbus/radio/header.h>

#include <cyng/log/record.h>
#include <cyng/io/io_buffer.h>
#include <cyng/obj/container_factory.hpp>

#include <iostream>

#ifdef _DEBUG_BROKER_WMBUS
#include <iostream>
#include <sstream>
#include <cyng/io/hex_dump.hpp>
#endif

#include <boost/uuid/nil_generator.hpp>

namespace smf {

	wmbus_session::wmbus_session(boost::asio::ip::tcp::socket socket, std::shared_ptr<db> db, cyng::logger logger, bus& cluster_bus)
	: socket_(std::move(socket))
		, logger_(logger)
		, db_(db)
		, bus_(cluster_bus)
		, buffer_()
		, buffer_write_()
		, parser_([this](mbus::radio::header const& h, cyng::buffer_t const& data) {
			this->decode(h, data);
		})
	{
	}

	wmbus_session::~wmbus_session()
	{
#ifdef _DEBUG_BROKER_WMBUS
		std::cout << "wmbus_session(~)" << std::endl;
#endif
	}


	void wmbus_session::stop()
	{
		//	https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload2.html
		CYNG_LOG_WARNING(logger_, "stop session");
		boost::system::error_code ec;
		socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
		socket_.close(ec);
	}

	void wmbus_session::start()
	{
		//
		//	start reading
		//
		do_read();

	}

	void wmbus_session::do_read() {
		auto self = shared_from_this();

		socket_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
			[this, self](boost::system::error_code ec, std::size_t bytes_transferred) {

				if (!ec) {
					CYNG_LOG_DEBUG(logger_, "[session] received " << bytes_transferred << " bytes from [" << socket_.remote_endpoint() << "]");

#ifdef _DEBUG_BROKER_WMBUS
					{
						std::stringstream ss;
						cyng::io::hex_dump<8> hd;
						hd(ss, buffer_.data(), buffer_.data() + bytes_transferred);
						CYNG_LOG_DEBUG(logger_, "[" << socket_.remote_endpoint() << "] " << bytes_transferred << " bytes:\n" << ss.str());
					}
#endif

					//
					//	let parse it
					//
					parser_.read(buffer_.data(), buffer_.data() + bytes_transferred);

					//
					//	continue reading
					//
					do_read();
				}
				else {
					CYNG_LOG_WARNING(logger_, "[session] read: " << ec.message());
				}

		});
	}

	void wmbus_session::do_write()
	{
		//if (is_stopped())	return;

		// Start an asynchronous operation to send a heartbeat message.
		boost::asio::async_write(socket_,
			boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
			std::bind(&wmbus_session::handle_write, this, std::placeholders::_1));
	}

	void wmbus_session::handle_write(const boost::system::error_code& ec)
	{
		//if (is_stopped())	return;

		if (!ec) {

			buffer_write_.pop_front();
			if (!buffer_write_.empty()) {
				do_write();
			}
			else {

				// Wait 10 seconds before sending the next heartbeat.
				//heartbeat_timer_.expires_after(boost::asio::chrono::seconds(10));
				//heartbeat_timer_.async_wait(std::bind(&bus::do_write, this));
			}
		}
		else {
			CYNG_LOG_ERROR(logger_, "[session] write: " << ec.message());

			//reset();
		}
	}

	void wmbus_session::decode(mbus::radio::header const& h, cyng::buffer_t const& data) {

		auto const flag_id = h.get_manufacturer_code();
		auto const manufacturer = mbus::decode(flag_id.first, flag_id.second);
		CYNG_LOG_TRACE(logger_, "[wmbus] meter: " << mbus::to_str(h) << " (" << manufacturer << ")");

		if (db_) {

			//
			//	if tag is "nil" no meter configuration was found
			//
			auto const[key, tag] = db_->lookup_meter(h.get_id());
			if (!tag.is_nil()) {

				//
				//	insert uplink data
				//
				bus_.req_db_insert_auto("wMBusUplink", cyng::data_generator(
					std::chrono::system_clock::now(),
					h.get_id(),	//	mbus::to_str(h),
					h.get_medium(),
					manufacturer,
					h.get_frame_type(),
					cyng::io::to_hex(data),	//	"payload",
					boost::uuids::nil_uuid()
				));

				//
				//	update config data
				//	ip adress, port and last seen
				//
				auto const ep = socket_.remote_endpoint();
				bus_.req_db_update("meterwMBus"
					, cyng::key_generator(tag)
					, cyng::param_map_factory()("address", ep.address())("port", ep.port())("lastSeen", std::chrono::system_clock::now()));
				//bus_.req_db_update("meterwMBus"
				//	, cyng::key_generator(tag)
				//	, cyng::param_map_factory()("port", ep.port()));
				//bus_.req_db_update("meterwMBus"
				//	, cyng::key_generator(tag)
				//	, cyng::param_map_factory()("lastSeen", std::chrono::system_clock::now()));

			}
			else {
				bus_.sys_msg(cyng::severity::LEVEL_WARNING, "[wmbus]", mbus::to_str(h), "has no AES key");
			}
		}
		else {
			CYNG_LOG_ERROR(logger_, "[wmbus] no database");
			bus_.sys_msg(cyng::severity::LEVEL_ERROR, "[wmbus] no database");
		}
	}

}


