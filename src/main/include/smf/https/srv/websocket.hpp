/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTPS_SRV_WEBSOCKET_HPP
#define NODE_LIB_HTTPS_SRV_WEBSOCKET_HPP

#include <smf/https/srv/https.h>
#include <NODE_project_info.h>
#include <cyng/vm/generator.h>
#include <cyng/json.h>
#include <boost/beast/websocket.hpp>
#include <boost/algorithm/string.hpp>

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
				, session_callback_t cb
				, boost::asio::io_context& ioc
				, std::vector<std::string> const& sub_protocols)
			: logger_(logger)
				, cb_(cb)
				, sub_protocols_(sub_protocols)
				, strand_(ioc.get_executor())
				, timer_(ioc, (std::chrono::steady_clock::time_point::max)())
			{
			}

			// Start the asynchronous operation
			template<class Body, class Allocator>
			void do_accept(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req)
			{
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

				// Accept the websocket handshake
				derived().ws().async_accept_ex(req, [&req, this](boost::beast::websocket::response_type& res) {

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

				}, boost::asio::bind_executor(strand_,
					std::bind(&websocket_session::on_accept
						, this
						, std::placeholders::_1)));

			}

			void on_accept(boost::system::error_code ec)
			{
				// Happens when the timer closes the socket
				if (ec == boost::asio::error::operation_aborted)
				{
					return;
				}

				if (ec)
				{
					CYNG_LOG_FATAL(logger_, "accept: " << ec.message());
					return;
				}

				// Read a message
				do_read();
			}

			// Called when the timer expires.
			void on_timer(boost::system::error_code ec)
			{
				if (ec && ec != boost::asio::error::operation_aborted)
				{
					CYNG_LOG_FATAL(logger_, "timer: " << ec.message());
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
									derived().shared_from_this(),
									std::placeholders::_1)));
					}
					else
					{
						// The timer expired while trying to handshake,
						// or we sent a ping and it never completed or
						// we never got back a control frame, so close.

						derived().do_timeout();
						return;
					}
				}

				// Wait on the timer
				timer_.async_wait(
					boost::asio::bind_executor(
						strand_,
						std::bind(
							&websocket_session::on_timer,
							derived().shared_from_this(),
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
			void on_ping(boost::system::error_code ec)
			{
				// Happens when the timer closes the socket
				if (ec == boost::asio::error::operation_aborted)
				{
					return;
				}

				if (ec)
				{
					CYNG_LOG_FATAL(logger_, "ping: " << ec.message());
					//return fail(ec, "ping");
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

			void do_read()
			{
				// Read a message into our buffer
				derived().ws().async_read(
					buffer_,
					boost::asio::bind_executor(
						strand_,
						std::bind(
							&websocket_session::on_read,
							derived().shared_from_this(),
							std::placeholders::_1,
							std::placeholders::_2)));
			}

			void on_read(boost::system::error_code ec, std::size_t bytes_transferred)
			{
				boost::ignore_unused(bytes_transferred);
				CYNG_LOG_TRACE(logger_, "received " 
					<< bytes_transferred
					<< " bytes websocket data" );

				// Happens when the timer closes the socket
				if (ec == boost::asio::error::operation_aborted)
				{
					return;
				}

				// This indicates that the websocket_session was closed
				if (ec == boost::beast::websocket::error::closed)
				{
					cb_(cyng::generate_invoke("ws.closed", ec));
					return;
				}

				if (ec)
				{
					CYNG_LOG_FATAL(logger_, "read: " << ec.message());
					return;
				}

				// Note that there is activity
				activity();

				std::stringstream msg;
				msg << boost::beast::buffers(buffer_.data());
				const std::string str = msg.str();
				CYNG_LOG_TRACE(logger_, "ws read - " << str);

				//
				//	expect JSON
				//
				cb_(cyng::generate_invoke("ws.read", cyng::json::read(str)));

				// Echo the message
				//derived().ws().text(derived().ws().got_text());
				//derived().ws().async_write(
				//	buffer_.data(),
				//	boost::asio::bind_executor(
				//		strand_,
				//		std::bind(
				//			&websocket_session::on_write,
				//			derived().shared_from_this(),
				//			std::placeholders::_1,
				//			std::placeholders::_2)));

				//
				// Clear buffer
				//
				derived().ws().text();
				derived().ws().text(derived().ws().got_text());

				buffer_.consume(buffer_.size());

				//
				//	continue reading
				//
				do_read();
			}

			void on_write(boost::system::error_code ec, std::size_t bytes_transferred)
			{
				boost::ignore_unused(bytes_transferred);

				// Happens when the timer closes the socket
				if (ec == boost::asio::error::operation_aborted)
				{
					return;
				}

				if (ec)
				{
					CYNG_LOG_FATAL(logger_, "write: " << ec.message());
					//return fail(ec, "write");
					return;
				}

				// Clear the buffer
				buffer_.consume(buffer_.size());

				// Do another read
				do_read();
			}

			/**
			 * @return session specific hash based on current address
			 */
			std::size_t hash() const noexcept
			{
				static std::hash<decltype(this)>	hasher;
				return hasher(this);
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
			session_callback_t cb_;
			std::vector<std::string> const& sub_protocols_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			boost::asio::steady_timer timer_;

		private:
			boost::beast::multi_buffer buffer_;
			char ping_state_ = 0;

		};
	}
}

#endif
