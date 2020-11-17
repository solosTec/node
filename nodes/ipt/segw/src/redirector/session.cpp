/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <redirector/session.h>
#include <cache.h>
#include <tasks/reflux.h>

#include <cyng/io/io_bytes.hpp>
#include <cyng/async/mux.h>
#include <cyng/async/task/task_builder.hpp>

#ifdef _DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

namespace node
{
	namespace redirector
	{
		session::session(boost::asio::ip::tcp::socket socket
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cache& cfg)
		: socket_(std::move(socket))
			, mux_(mux)
			, logger_(logger)
			, buffer_()
			, authorized_(false)
			, data_()
			, rx_(0)
			, sx_(0)
			, rs485_(cfg)
			, tsk_lmn_(cyng::async::NO_TASK)
			, tsk_reflux_(cyng::async::NO_TASK)
		{
			CYNG_LOG_INFO(logger_, "new redirector session ["
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
			//
			//	connect to RS 485 port
			//
			if (!rs485_.is_enabled()) {

				CYNG_LOG_WARNING(logger_, "RS-485 interface is not enabled - close session");
				boost::system::error_code ec;

#ifdef _DEBUG
				std::string str("RS-485: serial port is not enabled - close session");
				boost::asio::write(socket_, boost::asio::buffer(str.data(), str.size()), ec);
#endif

				socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ec);
				socket_.close(ec);
			}
			else {

				//
				//	sent data to serial interface
				//
				tsk_lmn_ = rs485_.get_lmn_task();
				CYNG_LOG_TRACE(logger_, "send incoming data from "
					<< socket_.remote_endpoint()
					<< " to #"
					<< tsk_lmn_);

				//
				//	get data from serial interface
				//
				auto const r = cyng::async::start_task_sync<reflux>(mux_
					, logger_
					, std::bind(&session::cb_data, this, std::placeholders::_1, std::placeholders::_2));
				if (r.second) {

					//
					//	add as listener
					//	ToDo: forward data to this session
					//
					tsk_reflux_ = r.first;
					mux_.post(tsk_lmn_, 1, cyng::tuple_factory(tsk_reflux_, true));

				}

			}

			//
			//	start reading from socket
			//
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
						CYNG_LOG_TRACE(logger_, "redirector session received "
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
						CYNG_LOG_WARNING(logger_, "redirector session closed <" << ec << ':' << ec.value() << ':' << ec.message() << '>');

						//
						//	stop task 
						//
						mux_.post(tsk_lmn_, 1, cyng::tuple_factory(tsk_reflux_, false));
						mux_.stop(tsk_reflux_);

						authorized_ = false;

					}
				});
		}

		void session::process_data(cyng::buffer_t&& data)
		{
			//
			//	send data to serial port
			//	
			mux_.post(tsk_lmn_, 0, cyng::tuple_factory(data)), 
			

			//
			//	update "sx" value of this session/device
			//
			sx_ += data.size();

		}

		void session::cb_data(cyng::buffer_t data, std::size_t msg_counter)
		{
			CYNG_LOG_INFO(logger_, "redirect "
				<< cyng::bytes_to_str(data.size())
				<< " to "
				<< socket_.remote_endpoint());

			boost::system::error_code ec;
			boost::asio::write(socket_, boost::asio::buffer(data.data(), data.size()), ec);
		}
	}
}
