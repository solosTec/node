/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/http/srv/websocket.h>
#include <smf/http/srv/connections.h>
#include <cyng/json.h>
#include <cyng/vm/generator.h>
#include <cyng/vm/domain/log_domain.h>
#include <boost/assert.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node
{
	namespace http
	{

		websocket_session::websocket_session(cyng::logging::log_ptr logger
			, connection_manager& cm
			, boost::asio::ip::tcp::socket&& socket
			, bus::shared_type bus
			, boost::uuids::uuid tag)
		: ws_(std::move(socket))
			, strand_(ws_.get_executor())
			, timer_(ws_.get_executor().context(), (std::chrono::steady_clock::time_point::max)())
			, buffer_()
			, logger_(logger)
			, connection_manager_(cm)
			, bus_(bus)
			, vm_(ws_.get_executor().context(), tag)
			, ping_cb_()
		{
			vm_.run(cyng::register_function("ws.send.json", 1, std::bind(&websocket_session::ws_send_json, this, std::placeholders::_1)));
            
            cyng::register_logger(logger_, vm_);
            vm_.run(cyng::generate_invoke("log.msg.info", "log domain is running"));
		}

		websocket_session::~websocket_session()
		{
// 			std::cerr << "websocket_session::~websocket_session()" << std::endl;
		}

		boost::uuids::uuid websocket_session::tag() const noexcept
		{
			return vm_.tag();
		}

		void websocket_session::on_accept(boost::system::error_code ec)
		{
			// Happens when the timer closes the socket
			if (ec == boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, "ws aborted - read");
				return;
			}

			if (ec)
			{
				CYNG_LOG_ERROR(logger_, "ws read error: " << ec << " - " << ec.message());
				return;
			}

			CYNG_LOG_TRACE(logger_, "ws accepted");

			// Read a message
			do_read();
		}

		// Called when the timer expires.
		void websocket_session::on_timer(boost::system::error_code ec)
		{
			if (ec && ec != boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, "ws timer aborted - read");
				return;
			}

			// See if the timer really expired since the deadline may have moved.
			if (timer_.expiry() <= std::chrono::steady_clock::now())
			{
				// If this is the first time the timer expired,
				// send a ping to see if the other end is there.
				if (ws_.is_open() && ping_state_ == 0)
				{
					// Note that we are sending a ping
					ping_state_ = 1;

					// Set the timer
					timer_.expires_after(std::chrono::seconds(15));

					// Now send the ping
					ws_.async_ping({},
						boost::asio::bind_executor(
							strand_,
							std::bind(
								&websocket_session::on_ping,
								this,
								//shared_from_this(),
								std::placeholders::_1)));
				}
				else
				{
					// The timer expired while trying to handshake,
					// or we sent a ping and it never completed or
					// we never got back a control frame, so close.

					// Closing the socket cancels all outstanding operations. They
					// will complete with boost::asio::error::operation_aborted
					//connection_manager_.stop(this);
					//do_close();
					//ws_.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
					//ws_.next_layer().close(ec);
					return;
				}
			}

			// Wait on the timer
			timer_.async_wait(boost::asio::bind_executor(strand_,
				std::bind(&websocket_session::on_timer
					, this
					, std::placeholders::_1)));
		}

		// Called to indicate activity from the remote peer
		void websocket_session::activity()
		{
			// Note that the connection is alive
			ping_state_ = 0;

			// Set the timer
			timer_.expires_after(std::chrono::seconds(15));
		}

		// Called after a ping is sent.
		void websocket_session::on_ping(boost::system::error_code ec)
		{
			// Happens when the timer closes the socket
			if (ec == boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, "ws aborted - ping");
				return;
			}

			if (ec)
			{
				CYNG_LOG_ERROR(logger_, "ping: " << ec << " - " << ec.message());
				return;
			}

			// Note that the ping was sent.
			if (ping_state_ == 1)
			{
				ping_state_ = 2;
			}
			else
			{
				// ping_state_ could have been set to 0
				// if an incoming control frame was received
				// at exactly the same time we sent a ping.
				BOOST_ASSERT(ping_state_ == 0);
			}
		}

		void websocket_session::on_control_callback(boost::beast::websocket::frame_type kind,
			boost::beast::string_view payload)
		{
			boost::ignore_unused(kind, payload);

			switch (kind)
			{
				/// A close frame was received
			case boost::beast::websocket::frame_type::close:
// 				CYNG_LOG_TRACE(logger_, "ws::close - " << payload);
                vm_.async_run(cyng::generate_invoke("log.msg.trace", "ws::close"));
				break;

				/// A ping frame was received
			case boost::beast::websocket::frame_type::ping:
// 				CYNG_LOG_TRACE(logger_, "ws::ping - " << payload);
                vm_.async_run(cyng::generate_invoke("log.msg.trace", "ws::ping"));
				break;

				/// A pong frame was received
			case boost::beast::websocket::frame_type::pong:
// 				CYNG_LOG_TRACE(logger_, "ws::pong - " << payload);
                vm_.async_run(cyng::generate_invoke("log.msg.trace", "ws::pong"));
				break;

			default:
				break;
			}


			// Note that there is activity
			activity();
		}

		void websocket_session::do_read()
		{
			// Read a message into our buffer
			ws_.async_read(	buffer_,
				boost::asio::bind_executor(strand_,
					std::bind(&websocket_session::on_read
						, this
						, std::placeholders::_1
						, std::placeholders::_2)));
		}

		void websocket_session::on_read(boost::system::error_code ec,
			std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);

			// Happens when the timer closes the socket
			if (ec == boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, "ws aborted - read");
				do_close();
				connection_manager_.stop(this);
				return;
			}

			// This indicates that the websocket_session was closed
			if (ec == boost::beast::websocket::error::closed)
			{
				CYNG_LOG_WARNING(logger_, "ws closed - read");
				do_close();
				connection_manager_.stop(this);
				return;
			}

			if (ec)
			{
				CYNG_LOG_ERROR(logger_, "ws read: " << ec << " - " << ec.message());
				timer_.cancel();
				connection_manager_.stop(this);
				return;
			}

			// Note that there is activity
			activity();

			std::stringstream msg;
			msg << boost::beast::buffers(buffer_.data());
			const std::string str = msg.str();
			CYNG_LOG_TRACE(logger_, "ws read - " << str);

			//
			//	execute on bus VM
			//
			bus_->vm_.run(cyng::generate_invoke("ws.read", vm_.tag(), cyng::invoke("ws.push"), cyng::json::read(str)));

			// Clear buffer
			ws_.text();
			ws_.text(ws_.got_text());

			buffer_.consume(buffer_.size());

			//std::stringstream ss;
			//ss << "{'tag' : '"
			//	<< tag_
			//	<< "'}";

			//ws_.async_write(boost::asio::buffer(ss.str()),
			//ws_.async_write(boost::asio::buffer("{'key' : 1}"),
			//	boost::asio::bind_executor(strand_,
			//		std::bind(&websocket_session::on_write,
			//			shared_from_this(),
			//			std::placeholders::_1,
			//			std::placeholders::_2)));
			do_read();
		}

		void websocket_session::on_write(boost::system::error_code ec,
			std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);

			// Happens when the timer closes the socket
			if (ec == boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, "ws aborted - write");
				do_close();
				connection_manager_.stop(this);
				return;
			}

			if (ec)
			{
				CYNG_LOG_ERROR(logger_, "ws write: " << ec << " - " << ec.message());
				do_close();
				connection_manager_.stop(this);
				return;
			}

			CYNG_LOG_TRACE(logger_, "ws write: " << bytes_transferred << " bytes");

			// Clear the buffer
			buffer_.consume(buffer_.size());

			// Do another read
			do_read();
		}

		void websocket_session::do_close()
		{
			CYNG_LOG_TRACE(logger_, "ws::do_close(cancel timer)");
			timer_.cancel();

			//CYNG_LOG_TRACE(logger_, "ws::do_close(use count " << shared_from_this().use_count() << ")");

			// Send a TCP shutdown
			boost::system::error_code ec;
			ws_.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			ws_.next_layer().close(ec);

			if (ec)
			{
				CYNG_LOG_WARNING(logger_, "ws::do_close(" << ec << ")");
			}

			// At this point the connection is closed gracefully
		}

		void websocket_session::run(cyng::vector_t&& prg)
		{
			//ws_.write(boost::asio::buffer("{'key' : 2}"));
			CYNG_LOG_TRACE(logger_, "run " << vm_.tag() << "... " << cyng::io::to_str(prg));
			vm_.run(std::move(prg));
			CYNG_LOG_TRACE(logger_, "...run " << vm_.tag());
		}

		void websocket_session::ws_send_json(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, "ws.send.json - " << cyng::io::to_str(frame));

			std::stringstream ss;
			cyng::json::write(ss, frame.at(0));
			ws_.write(boost::asio::buffer(ss.str()));
		}

		cyng::object make_websocket(cyng::logging::log_ptr logger
			, connection_manager& cm
			, boost::asio::ip::tcp::socket&& socket
			, bus::shared_type bus
			, boost::uuids::uuid tag)
		{
			return cyng::make_object<websocket_session>(logger
				, cm
				, std::move(socket)
				, bus
				, tag);
		}
	}
}

namespace cyng
{
	namespace traits
	{

#if defined(CYNG_LEGACY_MODE_ON)
		const char type_tag<node::http::websocket_session>::name[] = "ws";
#endif
	}	// traits	
}

namespace std
{
	size_t hash<node::http::websocket_session>::operator()(node::http::websocket_session const& t) const noexcept
	{
		return 0;
		//return std::hash<std::string>{}(t.meta().get_name());
	}

	bool equal_to<node::http::websocket_session>::operator()(node::http::websocket_session const& t1, node::http::websocket_session const& t2) const noexcept
	{
		return false;
		//return t1.meta().get_name() == t2.meta().get_name();
	}
}
