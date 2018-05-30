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
			, tag_(tag)
#if (BOOST_BEAST_VERSION < 167)
			, ping_cb_()
#endif
            , shutdown_(false)
		{
			//vm_.register_function("ws.send.json", 1, std::bind(&websocket_session::ws_send_json, this, std::placeholders::_1));
            
            //cyng::register_logger(logger_, vm_);
            //vm_.run(cyng::generate_invoke("log.msg.info", "log domain is running"));
		}

		websocket_session::~websocket_session()
		{
// 			std::cerr << "websocket_session::~websocket_session()" << std::endl;
		}

		boost::uuids::uuid websocket_session::tag() const noexcept
		{
			return tag_;
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
            //
            //  no activities after shutdown
            //
            if (shutdown_)  return;

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
					std::cerr << ec << std::endl;
                    CYNG_LOG_WARNING(logger_, "ws - incomplete ping");
                    do_close();
                    connection_manager_.stop(this);

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
			//timer_.async_wait(boost::asio::bind_executor(strand_,
			//	std::bind(&websocket_session::on_timer
			//		, this
			//		, std::placeholders::_1)));
		}

		// Called to indicate activity from the remote peer
		void websocket_session::activity()
		{
            //
            //  no activities after shutdown
            //
            if (shutdown_)  return;

            // Note that the connection is alive
			ping_state_ = 0;

			// Set the timer
//			timer_.expires_after(std::chrono::seconds(15));
		}

		// Called after a ping is sent.
		void websocket_session::on_ping(boost::system::error_code ec)
		{
            //
            //  no activities after shutdown
            //
            if (shutdown_)  return;

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
            //
            //  no activities after shutdown
            //
            if (shutdown_)  return;

            boost::ignore_unused(kind, payload);

			switch (kind)
			{
				/// A close frame was received
			case boost::beast::websocket::frame_type::close:
 				CYNG_LOG_TRACE(logger_, "ws::close - " << tag_);
				break;

				/// A ping frame was received
			case boost::beast::websocket::frame_type::ping:
 				CYNG_LOG_TRACE(logger_, "ws::ping - " << tag_);
				break;

				/// A pong frame was received
			case boost::beast::websocket::frame_type::pong:
 				CYNG_LOG_TRACE(logger_, "ws::pong - " << tag_);
				break;

			default:
				break;
			}


			// Note that there is activity
			activity();
		}

		void websocket_session::do_read()
		{
            //
            //  no activities after shutdown
            //
            if (shutdown_)  return;

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
            //
            //  no activities after shutdown
            //
            if (shutdown_)  return;

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
                do_close();
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
			bus_->vm_.async_run(cyng::generate_invoke("ws.read", tag_, cyng::invoke("ws.push"), cyng::json::read(str)));

			// Clear buffer
			ws_.text();
			ws_.text(ws_.got_text());

			buffer_.consume(buffer_.size());

			do_read();
		}

		void websocket_session::on_write(boost::system::error_code ec,
			std::size_t bytes_transferred)
		{
            //
            //  no activities after shutdown
            //
            if (shutdown_)  return;

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
            //
            //  no activities after shutdown
            //
            if (shutdown_)  return;

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

        void websocket_session::do_shutdown()
        {
            shutdown_ = true;

            timer_.cancel();

            if (ws_.is_open())
            {
                CYNG_LOG_WARNING(logger_, "ws::do_shutdown(" << ws_.next_layer().remote_endpoint() << ")");
                // Send a TCP shutdown
                boost::system::error_code ec;
                ws_.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                ws_.next_layer().close(ec);
            }
            else
            {
                CYNG_LOG_WARNING(logger_, "ws::do_shutdown(" << tag_ << ")");
            }

        }

		bool websocket_session::send_msg(std::string const& msg)
		{
            //
            //  no activities after shutdown
            //
            if (shutdown_)  return false;

            if (ws_.is_open())
			{
			    CYNG_LOG_TRACE(logger_, "ws.send.json... - " << msg);
			    ws_.write(boost::asio::buffer(msg));
			    CYNG_LOG_TRACE(logger_, "...ws.send.json");
				return true;
			}
		    CYNG_LOG_WARNING(logger_, "ws.send.json - closed " << msg);
			return false;
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
