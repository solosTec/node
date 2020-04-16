/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/http/srv/websocket.h>
#include <smf/http/srv/connections.h>
#include <smf/http/srv/generator.h>

#include <cyng/json.h>
#include <cyng/vm/generator.h>
#include <cyng/vm/domain/log_domain.h>
#include <cyng/table/key.hpp>

#include <boost/assert.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/functional/hash.hpp>

namespace node
{
	namespace http
	{
		websocket_session::websocket_session(cyng::logging::log_ptr logger
			, connections& cm
			, boost::asio::ip::tcp::socket&& socket
			, boost::uuids::uuid tag)
		: ws_(std::move(socket))
#if (BOOST_BEAST_VERSION < 248)
			, strand_(ws_.get_executor())
			, timer_(ws_.get_executor().context(), (std::chrono::steady_clock::time_point::max)())
#endif
			, buffer_()
			, logger_(logger)
			, connection_manager_(cm)
			, tag_(tag)
#if (BOOST_BEAST_VERSION < 167)
			, ping_cb_()
#endif
            , shutdown_(false)
		{
			setup_stream(ws_);
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
				CYNG_LOG_WARNING(logger_, tag() << " ws aborted - read");
				connection_manager_.stop_ws(tag());
				return;
			}

			if (ec)
			{
				CYNG_LOG_ERROR(logger_, tag() << " ws read error: " << ec << " - " << ec.message());
				connection_manager_.stop_ws(tag());
				return;
			}

			CYNG_LOG_TRACE(logger_, "ws accepted");

			// Read a message
			do_read();
		}

		// Called when the timer expires.
#if (BOOST_BEAST_VERSION < 248)
		void websocket_session::on_timer(boost::system::error_code ec, cyng::object obj)
		{
			BOOST_ASSERT(cyng::object_cast<websocket_session>(obj) == this);

            //
            //  no activities after shutdown
            //
            if (shutdown_)  return;

            if (ec && ec != boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, tag() << " ws timer aborted - read");
				connection_manager_.stop_ws(tag());
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
								std::placeholders::_1)));
				}
				else
				{
					// The timer expired while trying to handshake,
					// or we sent a ping and it never completed or
					// we never got back a control frame, so close.
					//std::cerr << ec << std::endl;
                    CYNG_LOG_WARNING(logger_, "ws - incomplete ping");
					connection_manager_.stop_ws(tag());

                    // Closing the socket cancels all outstanding operations. They
					// will complete with boost::asio::error::operation_aborted
					//ws_.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
					//ws_.next_layer().close(ec);
					return;
				}
			}

			// Wait on the timer
			timer_.async_wait(boost::asio::bind_executor(strand_,
				std::bind(&websocket_session::on_timer
					, this
					, std::placeholders::_1
					, obj)));
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
			timer_.expires_after(std::chrono::seconds(15));
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
				connection_manager_.stop_ws(tag());
				return;
			}

			if (ec)
			{
				CYNG_LOG_ERROR(logger_, "ping: " << ec << " - " << ec.message());
				connection_manager_.stop_ws(tag());
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
#endif

		void websocket_session::do_read()
		{
            //
            //  no activities after shutdown
            //
            if (shutdown_)  return;

            // Read a message into our buffer
#if (BOOST_BEAST_VERSION < 248)
			ws_.async_read(	buffer_,
				boost::asio::bind_executor(strand_,
					std::bind(&websocket_session::on_read
						, this
						, std::placeholders::_1
						, std::placeholders::_2)));
#else
			ws_.async_read(
				buffer_,
				boost::beast::bind_front_handler(
					&websocket_session::on_read,
					this));
#endif

		}

		void websocket_session::on_read(boost::system::error_code ec,
			std::size_t bytes_transferred)
		{
            //
            //  no activities after shutdown
            //
            if (shutdown_)  return;

			boost::ignore_unused(bytes_transferred);

#if (BOOST_BEAST_VERSION < 167)
			// Happens when the timer closes the socket
			if (ec == boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, "ws aborted - read");
				do_close();
				connection_manager_.stop_ws(tag());
				return;
			}
#endif

			// This indicates that the websocket_session was closed
			if (ec == boost::beast::websocket::error::closed)
			{
				CYNG_LOG_WARNING(logger_, "ws closed - read");
				do_close();
				connection_manager_.stop_ws(tag());
				return;
			}

			if (ec)
			{
                CYNG_LOG_ERROR(logger_, "ws read "
                    << tag()
                    << ": "
                    << ec
                    << " - "
                    << ec.message());
                do_close();
				connection_manager_.stop_ws(tag());
				return;
			}

			// Note that there is activity
#if (BOOST_BEAST_VERSION < 248)
			activity();
#endif

//#if (BOOST_BEAST_VERSION < 189)
#if (BOOST_BEAST_VERSION < 248)
			std::stringstream msg;
			msg << boost::beast::buffers(buffer_.data());
			std::string const str = msg.str();
//#elif (BOOST_BEAST_VERSION < 248)
			//std::string const str = boost::beast::buffers_to_string(boost::beast::buffers(buffer_.data()));
#else
			std::stringstream msg;
			msg << boost::beast::make_printable(buffer_.data());
			std::string const str = msg.str();
#endif
			
			CYNG_LOG_TRACE(logger_, "ws.read.json " << tag() << ": " << str);

			//
			//	update session status
			//
			connection_manager_.vm().async_run(node::db::modify_by_param("_HTTPSession"
				, cyng::table::key_generator(tag_)
				, cyng::param_factory("status", str)
				, 0u
				, connection_manager_.vm().tag()));

			//
			//	execute on bus VM
			//
			connection_manager_.vm().async_run(cyng::generate_invoke("ws.read", tag(), cyng::json::read(str)));

			// Clear buffer
			ws_.text();
			ws_.text(ws_.got_text());

			buffer_.consume(buffer_.size());

			//
			//	continue to read incoming data
			//
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

#if (BOOST_BEAST_VERSION < 167)
			// Happens when the timer closes the socket
			if (ec == boost::asio::error::operation_aborted)
			{
				CYNG_LOG_WARNING(logger_, "ws aborted - write");
				do_close();
				connection_manager_.stop_ws(tag());
				return;
			}
#endif

			if (ec)
			{
				CYNG_LOG_ERROR(logger_, "ws write: " << ec << " - " << ec.message());
				do_close();
				connection_manager_.stop_ws(tag());
				return;
			}

			CYNG_LOG_TRACE(logger_, "ws write: " << bytes_transferred << " bytes");

			// Clear the buffer
			buffer_.consume(buffer_.size());

			//
			//	already triggered by on_read()
			//
			//do_read();
		}

		void websocket_session::do_close()
		{
            //
            //  no activities after shutdown
            //
            if (shutdown_)  return;

			boost::system::error_code ec;

#if (BOOST_BEAST_VERSION < 167)
			CYNG_LOG_TRACE(logger_, "ws::do_close(cancel timer)");
			timer_.cancel();
#endif

			//CYNG_LOG_TRACE(logger_, "ws::do_close(use count " << shared_from_this().use_count() << ")");

			// Send a TCP shutdown
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

#if (BOOST_BEAST_VERSION < 167)
			timer_.cancel();
#endif

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
			    CYNG_LOG_TRACE(logger_, "ws.write.json " << tag() << ": " << msg);
				//ws_.lowest_layer().wait(boost::asio::ip::tcp::socket::wait_write);
			    ws_.write(boost::asio::buffer(msg));
				return true;
			}
		    CYNG_LOG_WARNING(logger_, "ws.write.json " << tag() << " closed " << msg);
			return false;
		}
	}
}

namespace cyng
{
	namespace traits
	{

#if !defined(__CPP_SUPPORT_N2235)
		const char type_tag<node::http::websocket_session>::name[] = "plain-websocket";
#endif
	}	// traits	
}

namespace std
{
	size_t hash<node::http::websocket_session>::operator()(node::http::websocket_session const& t) const noexcept
	{
		return boost::hash<boost::uuids::uuid>()(t.tag());
	}

	bool equal_to<node::http::websocket_session>::operator()(node::http::websocket_session const& t1, node::http::websocket_session const& t2) const noexcept
	{
		return t1.tag() == t2.tag();
	}

	bool less<node::http::websocket_session>::operator()(node::http::websocket_session const& t1, node::http::websocket_session const& t2) const noexcept
	{
		return t1.tag() < t2.tag();
	}

}
