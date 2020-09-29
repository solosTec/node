/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "session.h"
#include <smf/cluster/generator.h>
#include <smf/sml/srv_id_io.h>

#include <cyng/io/io_bytes.hpp>
#include <cyng/io/io_buffer.h>
#include <cyng/io/hex_dump.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/controller.h>
#include <cyng/util/split.h>
#include <cyng/table/key.hpp>
#include <cyng/table/body.hpp>

namespace node
{
	namespace nms
	{
		session::session(boost::asio::ip::tcp::socket socket
			, cyng::logging::log_ptr logger)
			//, cyng::controller& cluster
			//, cyng::controller& vm)
		: socket_(std::move(socket))
			, logger_(logger)
			//, cluster_(cluster)
			//, vm_(vm)
			, buffer_()
			, authorized_(false)
			, data_()
			, rx_(0)
			, sx_(0)
			//, parser_([&](cyng::vector_t&& prg) {

			//	//	auto const str = cyng::io::to_hex(data);
			//	//	auto const srv_id = sml::from_server_id(h.get_server_id());

			//	CYNG_LOG_TRACE(logger_, vm_.tag() << ": " << cyng::io::to_str(prg));
			//	}, true)
		{
			CYNG_LOG_INFO(logger_, "new session ["
				//<< vm_.tag()
				<< "] at "
				<< socket_.remote_endpoint());

		}

		session::~session()
		{
			//
			//	remove from session list
			//
		}

		void session::start()
		{
			do_read();
		}

		void session::do_read()
		{
			auto self(shared_from_this());

			socket_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
				[this, self](boost::system::error_code ec, std::size_t bytes_transferred)
				{
					if (!ec)
					{
						CYNG_LOG_TRACE(logger_, "session received "
							<< cyng::bytes_to_str(bytes_transferred));

#ifdef _DEBUG
						cyng::io::hex_dump hd;
						std::stringstream ss;
						hd(ss, buffer_.begin(), buffer_.begin() + bytes_transferred);
						CYNG_LOG_TRACE(logger_, "\n" << ss.str());
#endif
						if (authorized_) {
							//
							//	raw data
							//
							process_data(cyng::buffer_t{ buffer_.begin(), buffer_.begin() + bytes_transferred });
						}
						else {

							//
							//	login data
							//
							process_login(cyng::buffer_t{ buffer_.begin(), buffer_.begin() + bytes_transferred });
						}

						//
						//	continue reading
						//
						do_read();
					}
					else
					{
						//	leave
						CYNG_LOG_WARNING(logger_, "NMS session closed <" << ec << ':' << ec.value() << ':' << ec.message() << '>');

						//
						//	remove client
						//
						//cluster_.async_run(client_req_close(vm_.tag(), ec.value()));

						authorized_ = false;

					}
				});
		}

		void session::process_data(cyng::buffer_t&& data)
		{
			//
			//	insert new record into "_wMBusUplink" table
			//	
			//parser_.read(data.begin(), data.end());

			//
			//	update "sx" value of this session/device
			//
			sx_ += data.size();
			//cluster_.async_run(bus_req_db_modify("_Session"
			//	, cyng::table::key_generator(vm_.tag())
			//	, cyng::param_factory("sx", sx_)
			//	, 0u
			//	, vm_.tag()));


		}

		void session::process_login(cyng::buffer_t&& data)
		{
			data_.insert(data_.end(), data.begin(), data.end());

			auto pos = std::find(data_.begin(), data_.end(), '\n');
			if (pos != data_.end()) {

				auto const vec = cyng::split(std::string(data_.begin(), pos), ":");
				if (vec.size() == 2) {

					CYNG_LOG_INFO(logger_, "send authorization request to cluster: "
						<< vec.at(0)
						<< ':'
						<< vec.at(1)
						<< '@'
						<< socket_.remote_endpoint());

					//cluster_.async_run(client_req_login(vm_.tag()
					//	, vec.at(0)
					//	, vec.at(1)
					//	, "plain" //	login scheme
					//	, cyng::param_map_factory("tp-layer", "TCP/IP")
					//	("data-layer", "IEC-62056-21:2002")
					//	("security", "public")
					//	("time", std::chrono::system_clock::now())
					//	("local-ep", socket_.remote_endpoint())
					//	("remote-ep", socket_.local_endpoint())));

				}

				//
				//	move iterator behind '\n' and
				//	remove login data from data stream
				//
				if (pos != data_.end())	++pos;
				data_.erase(data_.begin(), pos);

				//
				//	login complete
				//
				authorized_ = true;

				if (!data_.empty()) {
					process_data(std::move(data_));
				}
			}
			else {
				if (data_.size() > 256) {
					CYNG_LOG_WARNING(logger_, "give up on waiting for login after "
						<< cyng::bytes_to_str(data_.size()));
					authorized_ = true;
				}
			}
		}
	}
}
