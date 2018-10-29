/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTP_SRV_WEBSOCKET_H
#define NODE_LIB_HTTP_SRV_WEBSOCKET_H

#include <NODE_project_info.h>

#include <cyng/log.h>
#include <cyng/object.h>

#include <boost/beast/websocket.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>

namespace node
{
	namespace http
	{
		class connections;
		class websocket_session
		{
		public:
			// Take ownership of the socket
			explicit websocket_session(cyng::logging::log_ptr
				, connections& cm
				, boost::asio::ip::tcp::socket&& socket
				, boost::uuids::uuid);

			virtual  ~websocket_session();

			/**
			 * @return VM tag
			 */
			boost::uuids::uuid tag() const noexcept;

			// Start the asynchronous operation
			template<class Body, class Allocator>
			void do_accept(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req, cyng::object obj)
			{
				BOOST_ASSERT(cyng::object_cast<websocket_session>(obj) == this);

				// Set the control callback. This will be called
				// on every incoming ping, pong, and close frame.
				//
				//  API change
				//
#if (BOOST_BEAST_VERSION < 167)
				//
				//	This mess will be fixed with Boost 1.67
				//	see: https://github.com/boostorg/beast/issues/1054
				//
				ping_cb_ = std::bind(&websocket_session::on_control_callback,
						this,
						std::placeholders::_1,
						std::placeholders::_2);
				ws_.control_callback(ping_cb_);

#else
				ws_.control_callback(std::bind(&websocket_session::on_control_callback
					, this
					, std::placeholders::_1
					, std::placeholders::_2));

#endif
				// Run the timer. The timer is operated
				// continuously, this simplifies the code.
				on_timer(boost::system::error_code{}, obj);

				// Set the timer
				timer_.expires_after(std::chrono::seconds(15));

				//
				//	check subprotocols
				//
				const auto pos = req.find(boost::beast::http::field::sec_websocket_protocol);
				if (pos != req.end())
				{
					CYNG_LOG_TRACE(logger_, "ws subprotocol(s) available: " << pos->value());
					ws_.async_accept_ex(req, [&req, pos, this](boost::beast::websocket::response_type& res) {

						//
						//	user agent
						//
						res.set(boost::beast::http::field::user_agent, NODE::version_string);
						res.set(boost::beast::http::field::server, NODE::version_string);

						//
						//	accept a subprotocol
						//
						std::vector<std::string> subprotocols;
						boost::algorithm::split(subprotocols, pos->value(), boost::is_any_of(","), boost::token_compress_on);

						for (auto idx = subprotocols.begin(); idx != subprotocols.end(); idx++)
						{
							CYNG_LOG_TRACE(logger_, "subprotocol: " << *idx );
							if (boost::algorithm::equals(*idx, "SMF"))
							{
								res.set(boost::beast::http::field::sec_websocket_protocol, *idx);
							}
						}

					}, boost::asio::bind_executor(strand_,
						std::bind(&websocket_session::on_accept
							, this
							, std::placeholders::_1)));
				}
				else
				{
					// Accept the websocket handshake
					ws_.async_accept(req,
						boost::asio::bind_executor(	strand_,
							std::bind(&websocket_session::on_accept
								, this
								, std::placeholders::_1)));
				}
			}

			void on_accept(boost::system::error_code ec);


			// Called after a ping is sent.
			void on_ping(boost::system::error_code ec);

			void on_control_callback(boost::beast::websocket::frame_type kind,
				boost::beast::string_view payload);

			void do_read();
			void on_read(boost::system::error_code ec, std::size_t bytes_transferred);
			void on_write(boost::system::error_code ec, std::size_t bytes_transferred);
			void do_close();
            void do_shutdown();

            /**
             * Send a (JSON) message to client
             */
            bool send_msg(std::string const&);

		private:
			/**
			 * Called to indicate activity from the remote peer
			 */
			void activity();

			/**
			 * Called when the timer expires.
			 */
			void on_timer(boost::system::error_code ec, cyng::object obj);

			//void ws_send_json(cyng::context& ctx);

		private:
			boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			boost::asio::steady_timer timer_;
			boost::beast::multi_buffer buffer_;
			cyng::logging::log_ptr logger_;
			connections& connection_manager_;
			boost::uuids::uuid tag_;
			char ping_state_ = 0;
#if (BOOST_BEAST_VERSION < 167)
			std::function<void(boost::beast::websocket::frame_type, boost::beast::string_view)>	ping_cb_;
#endif
            bool shutdown_;
		};

	}
}

#include <cyng/intrinsics/traits.hpp>
#include <cyng/intrinsics/traits/tag.hpp>

namespace cyng
{
	namespace traits
	{
		template <>
		struct type_tag<node::http::websocket_session>
		{
			using type = node::http::websocket_session;
			using tag = std::integral_constant<std::size_t, PREDEF_SOCKET>;
#if defined(CYNG_LEGACY_MODE_ON)
			const static char name[];
#else
			constexpr static char name[] = "plain-websocket";
#endif
		};

		template <>
		struct reverse_type < PREDEF_SOCKET >
		{
			using type = node::http::websocket_session;
		};
	}

}

namespace std
{
	template<>
	struct hash<node::http::websocket_session>
	{
		size_t operator()(node::http::websocket_session const& t) const noexcept;
	};

	template<>
	struct equal_to<node::http::websocket_session>
	{
		//	pre C++17
		using result_type = bool;
		using first_argument_type = node::http::websocket_session;
		using second_argument_type = node::http::websocket_session;

		bool operator()(node::http::websocket_session const& t1, node::http::websocket_session const& t2) const noexcept;
	};

	template<>
	struct less<node::http::websocket_session>
	{
		//	pre C++17
		using result_type = bool;
		using first_argument_type = node::http::websocket_session;
		using second_argument_type = node::http::websocket_session;

		bool operator()(node::http::websocket_session const& t1, node::http::websocket_session const& t2) const noexcept;
	};

}
#endif
