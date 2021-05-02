/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <wmbus_session.h>
#include <tasks/gatekeeper.h>

#include <smf/mbus/flag_id.h>
#include <smf/mbus/radio/header.h>
#include <smf/mbus/radio/decode.h>

#include <cyng/log/record.h>
#include <cyng/io/io_buffer.h>
#include <cyng/obj/container_factory.hpp>

#include <iostream>

#ifdef _DEBUG_BROKER_WMBUS
#include <iostream>
#include <sstream>
#include <cyng/io/hex_dump.hpp>
#include <smf/sml/unpack.h>
#endif

#include <boost/uuid/nil_generator.hpp>

namespace smf {

	wmbus_session::wmbus_session(cyng::controller& ctl
		, boost::asio::ip::tcp::socket socket
		, std::shared_ptr<db> db
		, cyng::logger logger
		, bus& cluster_bus)
	: ctl_(ctl)
		, socket_(std::move(socket))
		, logger_(logger)
		, db_(db)
		, bus_(cluster_bus)
		, buffer_()
		, buffer_write_()
		, parser_([this](mbus::radio::header const& h, mbus::radio::tpl const& t, cyng::buffer_t const& data) {
			this->decode(h, t, data);
		})
		, gatekeeper_()
	{	}

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

	void wmbus_session::start(std::chrono::seconds timeout)
	{
		//
		//	start reading
		//
		do_read();

		gatekeeper_ = ctl_.create_channel_with_ref<gatekeeper>(logger_, timeout, this->shared_from_this());

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
					gatekeeper_->stop();
				}

		});
	}

	void wmbus_session::do_write()
	{
		// Start an asynchronous operation to send a heartbeat message.
		boost::asio::async_write(socket_,
			boost::asio::buffer(buffer_write_.front().data(), buffer_write_.front().size()),
			std::bind(&wmbus_session::handle_write, this, std::placeholders::_1));
	}

	void wmbus_session::handle_write(const boost::system::error_code& ec)
	{
		if (!ec) {

			buffer_write_.pop_front();
			if (!buffer_write_.empty()) {
				do_write();
			}
		}
		else {
			CYNG_LOG_ERROR(logger_, "[session] write: " << ec.message());
		}
	}

	void wmbus_session::decode(mbus::radio::header const& h, mbus::radio::tpl const& t, cyng::buffer_t const& data) {

		CYNG_LOG_TRACE(logger_, "[wmbus] meter: " << mbus::to_str(h));

		if (h.has_secondary_address()) {
			decode(t.get_secondary_address(), t.get_access_no(), h.get_frame_type(), data);
		}
		else {
			decode(h.get_server_id(), t.get_access_no(), h.get_frame_type(), data);
		}
	}

	void wmbus_session::decode(srv_id_t address
		, std::uint8_t access_no
		, std::uint8_t frame_type
		, cyng::buffer_t const& data) {

		auto const flag_id = get_manufacturer_code(address);
		auto const manufacturer = mbus::decode(flag_id.first, flag_id.second);

		if (db_) {

			//
			//	if tag is "nil" no meter configuration was found
			//
			auto const [key, tag] = db_->lookup_meter(get_id(address));
			if (!tag.is_nil()) {

				auto const payload = mbus::radio::decode(address
					, access_no
					, key
					, data);

				//
				//	insert uplink data
				//
				bus_.req_db_insert_auto("wMBusUplink", cyng::data_generator(
					std::chrono::system_clock::now(),
					get_id(address),	//	mbus::to_str(h),
					get_medium(address),
					manufacturer,
					frame_type,
					cyng::io::to_hex(payload),	//	"payload",
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

				switch (frame_type) {
				case mbus::FIELD_CI_RES_LONG_SML:	//	0x7E - long header
				case mbus::FIELD_CI_RES_SHORT_SML:	//	0x7F - short header
					push_sml_data(payload);
					break;
				case mbus::FIELD_CI_RES_LONG_DLMS:	// 0x7C
				case mbus::FIELD_CI_RES_SHORT_DLSM: //	0x7D - short header
					push_dlsm_data(payload);
					break;
				case mbus::FIELD_CI_HEADER_LONG:	//	0x72 - 12 byte header followed by variable format data (EN 13757-3)
				case mbus::FIELD_CI_HEADER_SHORT:	//	0x7A - 4 byte header followed by variable format data (EN 13757-3)
					push_data(payload);
					break;

				default:
					break;
				}

			}
			else {
				bus_.sys_msg(cyng::severity::LEVEL_WARNING, "[wmbus]", srv_id_to_str(address), "has no AES key");
			}
		}
		else {
			CYNG_LOG_ERROR(logger_, "[wmbus] no database");
			bus_.sys_msg(cyng::severity::LEVEL_ERROR, "[wmbus] no database");
		}
	}

	void wmbus_session::push_sml_data(cyng::buffer_t const& payload) {
#ifdef _DEBUG_BROKER_WMBUS
		//
		//	read SML data
		// 
		smf::sml::unpack p([this](std::string trx, std::uint8_t, std::uint8_t, smf::sml::msg_type type, cyng::tuple_t msg, std::uint16_t crc) {

			CYNG_LOG_DEBUG(logger_, "SML> " << smf::sml::get_name(type) << ": " << trx << ", " << msg);

			});
		p.read(payload.begin() + 2, payload.end());
#endif

	}
	void wmbus_session::push_dlsm_data(cyng::buffer_t const& payload) {

	}
	void wmbus_session::push_data(cyng::buffer_t const& payload) {

	}

}


