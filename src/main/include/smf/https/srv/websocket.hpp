/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTPS_SRV_WEBSOCKET_HPP
#define NODE_LIB_HTTPS_SRV_WEBSOCKET_HPP

#include <smf/https/srv/https.h>
#include <smf/https/srv/connections.h>
#include <NODE_project_info.h>

#include <cyng/vm/generator.h>
#include <cyng/vm/controller.h>
#include <cyng/json.h>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

//#if (BOOST_VERSION < 107000)
//#if (BOOST_ASIO_VERSION < 101202)
#if (BOOST_BEAST_VERSION < 248)
#include <boost/beast/experimental/core/ssl_stream.hpp>
#else
#include <boost/beast/ssl/ssl_stream.hpp>
#endif
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace node
{
	namespace https
	{
		// Echoes back all received WebSocket messages.
		// This uses the Curiously Recurring Template Pattern so that
		// the same code works with both SSL streams and regular sockets.
		template<class Derived>
		class websocket_session
		{
			// Access the derived class, this is part of
			// the Curiously Recurring Template Pattern idiom.
			Derived& derived()
			{
				return static_cast<Derived&>(*this);
			}

		public:
			// Construct the session
			explicit websocket_session(cyng::logging::log_ptr logger
				, connections& cm
				, boost::uuids::uuid tag
#if (BOOST_BEAST_VERSION < 248)
				, boost::asio::io_context& ioc
#endif
			)
			: logger_(logger)
				, connection_manager_(cm)
				, tag_(tag)
				, sub_protocols_{"SMF", "LoRa"}
#if (BOOST_BEAST_VERSION < 248)
				, strand_(ioc.get_executor())
				, timer_(ioc, (std::chrono::steady_clock::time_point::max)())
#endif
				, buffer_()
			{}

			boost::uuids::uuid tag() const
			{
				return tag_;
			}

#if (BOOST_BEAST_VERSION >= 248)
			// Start the asynchronous operation
			template<class Body, class Allocator>
			void run(cyng::object obj, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req)
			{
				// Accept the WebSocket upgrade request
				do_accept(obj, std::move(req));
			}
#endif

			// Start the asynchronous operation
			template<class Body, class Allocator>
			void do_accept(cyng::object obj, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req)
			{
#if (BOOST_BEAST_VERSION < 248)
				// Set the control callback. This will be called
				// on every incoming ping, pong, and close frame.
				derived().ws().control_callback(
					std::bind(
						&websocket_session::on_control_callback,
						this,
						std::placeholders::_1,
						std::placeholders::_2));

				// Set the timer
				timer_.expires_after(std::chrono::seconds(15));
#else
				// Set suggested timeout settings for the websocket
				derived().ws().set_option(
					boost::beast::websocket::stream_base::timeout::suggested(
						boost::beast::role_type::server));
#endif

				//
				// Accept the websocket handshake
				//
				derived().ws().async_accept_ex(req, [&req, this, obj](boost::beast::websocket::response_type& res) {

					//
					//	user agent, subprotocol
					//
					res.set(boost::beast::http::field::user_agent, NODE::version_string);
					res.set(boost::beast::http::field::server, NODE::version_string);

					//
					//	check subprotocols
					//
					const auto pos = req.find(boost::beast::http::field::sec_websocket_protocol);
					if (pos != req.end())
					{
						CYNG_LOG_TRACE(logger_, "ws subprotocol(s) requested: " << pos->value());
						auto match = search_sub_protocol(pos->value());
						if (!match.empty())
						{
							CYNG_LOG_TRACE(logger_, "matching subprotocol " << match);
							res.set(boost::beast::http::field::sec_websocket_protocol, match);
						}
					}

				}, 
#if (BOOST_BEAST_VERSION < 248)
					boost::asio::bind_executor(strand_,
					std::bind(&websocket_session::on_accept
						, this
						, obj //	reference
						, std::placeholders::_1)));
#else
					boost::beast::bind_front_handler(
						&websocket_session::on_accept,
						this,
						obj));
#endif
			}

			void on_accept(cyng::object obj, boost::system::error_code ec)
			{
#if (BOOST_BEAST_VERSION < 248)
				// Happens when the timer closes the socket
				if (ec == boost::asio::error::operation_aborted)	{
					return;
				}
#endif

				if (ec)	{
					CYNG_LOG_FATAL(logger_, "ws " << tag() << " accept: " << ec.message());
					return;
				}

				// Read a message
				do_read(obj);
			}

#if (BOOST_BEAST_VERSION < 248)
			// Called when the timer expires.
			void on_timer(cyng::object obj, boost::system::error_code ec)
			{
				if (ec && ec != boost::asio::error::operation_aborted)
				{
					CYNG_LOG_FATAL(logger_, "ws " << tag() << " timer: " << ec.message());
					return;
				}

				// See if the timer really expired since the deadline may have moved.
				if (timer_.expiry() <= std::chrono::steady_clock::now())
				{
					// If this is the first time the timer expired,
					// send a ping to see if the other end is there.
					if (derived().ws().is_open() && ping_state_ == 0)
					{
						// Note that we are sending a ping
						ping_state_ = 1;

						// Set the timer
						timer_.expires_after(std::chrono::seconds(15));

						// Now send the ping
						derived().ws().async_ping({},
							boost::asio::bind_executor(
								strand_,
								std::bind(
									&websocket_session::on_ping,
									//derived().shared_from_this(),
									&derived(),
									obj,	//	reference
									std::placeholders::_1)));
					}
					else
					{
						// The timer expired while trying to handshake,
						// or we sent a ping and it never completed or
						// we never got back a control frame, so close.

						derived().do_timeout(obj);
						return;
					}
				}

				// Wait on the timer
				timer_.async_wait(
					boost::asio::bind_executor(
						strand_,
						std::bind(
							&websocket_session::on_timer,
							//derived().shared_from_this(),
							&derived(),
							obj,	//	reference
							std::placeholders::_1)));
			}

			// Called to indicate activity from the remote peer
			void activity()
			{
				// Note that the connection is alive
				ping_state_ = 0;

				// Set the timer
				timer_.expires_after(std::chrono::seconds(15));
			}

			// Called after a ping is sent.
			void on_ping(cyng::object obj, boost::system::error_code ec)
			{
				// Happens when the timer closes the socket
				if (ec == boost::asio::error::operation_aborted)
				{
					connection_manager_.stop_ws(tag());
					return;
				}

				if (ec)
				{
					CYNG_LOG_FATAL(logger_, "ws " << tag() << " ping: " << ec.message());
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

			void on_control_callback(boost::beast::websocket::frame_type kind,
					boost::beast::string_view payload)
			{
				boost::ignore_unused(kind, payload);

				// Note that there is activity
				activity();
			}
#endif

			void do_read(cyng::object obj)
			{
				// Read a message into our buffer
#if (BOOST_BEAST_VERSION < 248)
				derived().ws().async_read(
					buffer_,
					boost::asio::bind_executor(
						strand_,
						std::bind(
							&websocket_session::on_read,
							//derived().shared_from_this(),
							&derived(),
							obj,	//	reference
							std::placeholders::_1,
							std::placeholders::_2)));
#else
				derived().ws().async_read(
					buffer_,
					boost::beast::bind_front_handler(
						&websocket_session::on_read,
						&derived(),
						obj));	//	reference
#endif
			}

			void on_read(cyng::object obj, boost::system::error_code ec, std::size_t bytes_transferred)
			{
				boost::ignore_unused(bytes_transferred);
				CYNG_LOG_TRACE(logger_, "ws "
					<< tag_ 
					<< " received " 
					<< bytes_transferred
					<< " bytes websocket data" );

#if (BOOST_BEAST_VERSION < 248)
				// Happens when the timer closes the socket
				if (ec == boost::asio::error::operation_aborted)	{
					connection_manager_.stop_ws(tag());
					return;
				}
#endif

				// This indicates that the websocket_session was closed
				if (ec == boost::beast::websocket::error::closed)
				{
					//
					//	remove websocket
					//
					connection_manager_.stop_ws(tag());
					return;
				}

				if (ec)
				{
					CYNG_LOG_FATAL(logger_, "ws " << tag() << " read: " << ec.message());
					connection_manager_.stop_ws(tag());
					return;
				}

#if (BOOST_BEAST_VERSION < 248)
				// Note that there is activity
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
				CYNG_LOG_TRACE(logger_, "ws " << tag() << " read - " << str);

				//
				//	expect JSON
				//
				connection_manager_.vm().async_run(cyng::generate_invoke("ws.read", tag(), cyng::json::read(str)));

				//
				// Clear buffer
				//
				derived().ws().text();
				derived().ws().text(derived().ws().got_text());

				buffer_.consume(buffer_.size());

				//
				//	continue reading
				//
				do_read(obj);
			}

			void on_write(boost::system::error_code ec, std::size_t bytes_transferred)
			{
				boost::ignore_unused(bytes_transferred);

#if (BOOST_BEAST_VERSION < 248)
				// Happens when the timer closes the socket
				if (ec == boost::asio::error::operation_aborted)	{
					return;
				}
#endif

				if (ec)	{
					CYNG_LOG_FATAL(logger_, "ws " << tag() << " write: " << ec.message());
					connection_manager_.stop_ws(tag());
					return;
				}

				// Clear the buffer
				buffer_.consume(buffer_.size());

				// Do another read
				//do_read();
			}

			/**
			 * @return session specific hash based on current address
			 */
			std::size_t hash() const noexcept
			{
				static std::hash<decltype(this)>	hasher;
				return hasher(this);
			}

			/**
			 * Write the specified message sync
			 */
			bool write(std::string const& msg)
			{

				if (derived().ws().is_open())
				{
					CYNG_LOG_TRACE(logger_, "ws " << tag() << " write: " << msg);
					derived().ws().write(boost::asio::buffer(msg));
					return true;
				}
				CYNG_LOG_WARNING(logger_, "ws " << tag() << " write (closed): " << msg);
				return false;
			}

		private:
			std::string search_sub_protocol(boost::beast::string_view value)
			{
				//
				//	get subprotocols
				//
				std::vector<std::string> subprotocols;
				boost::algorithm::split(subprotocols, value, boost::is_any_of(","), boost::token_compress_on);

				//
				//	search matching subprotocols
				//
				for (auto sub : subprotocols)
				{
					auto pos = std::find(sub_protocols_.begin(), sub_protocols_.end(), sub);
					if (pos != sub_protocols_.end())
					{
						return *pos;
					}
				}
				return "";
			}

		protected:
			cyng::logging::log_ptr logger_;
			connections& connection_manager_;
			const boost::uuids::uuid tag_;
			const std::vector<std::string> sub_protocols_;
#if (BOOST_BEAST_VERSION < 248)
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			boost::asio::steady_timer timer_;
#endif

		private:
#if (BOOST_BEAST_VERSION < 248)
			boost::beast::multi_buffer buffer_;
			char ping_state_ = 0;
#else
			boost::beast::flat_buffer buffer_;
#endif

		};
	}
}

#endif
