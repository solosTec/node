/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "session.h"
#include "../cache.h"
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
#include <cyng/set_cast.h>

namespace node
{
	namespace nms
	{
		session::session(boost::asio::ip::tcp::socket socket
			, cyng::logging::log_ptr logger
			, cache& cfg)
		: socket_(std::move(socket))
			, logger_(logger)
			, reader_(logger, cfg)
			, buffer_()
			, authorized_(false)
			, data_()
			, rx_(0)
			, sx_(0)
			, parser_([&](cyng::vector_t&& data) {
				CYNG_LOG_TRACE(logger_, cyng::io::to_str(data));
				for (auto const& obj : data) {
					reader_.run(cyng::to_param_map(obj));
				}
			})
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
						//
						//	feeding the parser
						//
						process_data(cyng::buffer_t{ buffer_.begin(), buffer_.begin() + bytes_transferred });

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
			parser_.read(data.begin(), data.end());

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

		reader::reader(cyng::logging::log_ptr logger, cache& cfg)
			: logger_(logger)
			, cache_(cfg)
		{}

		void reader::run(cyng::param_map_t&& pm)
		{
			CYNG_LOG_DEBUG(logger_, "NMS request: " << cyng::io::to_type(pm));
		}

	}
}
