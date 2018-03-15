/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTP_SRV_WEBSOCKET_H
#define NODE_LIB_HTTP_SRV_WEBSOCKET_H

#include <smf/cluster/bus.h>
#include <cyng/log.h>
#include <cyng/vm/controller.h>
#include <boost/beast/websocket.hpp>
#include <boost/asio/steady_timer.hpp>
#include <cyng/log.h>

namespace node
{
	namespace http
	{
		class connection_manager;
		class websocket_session
		{
		public:
			// Take ownership of the socket
			explicit websocket_session(cyng::logging::log_ptr
				, connection_manager& cm
				, boost::asio::ip::tcp::socket&& socket
				, bus::shared_type
				, boost::uuids::uuid);

			virtual  ~websocket_session();

			/**
			 * @return VM tag
			 */
			boost::uuids::uuid tag() const noexcept;

			// Start the asynchronous operation
			template<class Body, class Allocator>
			void do_accept(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req)
			{
				// Set the control callback. This will be called
				// on every incoming ping, pong, and close frame.
				//	Buggy - don't call this
//#if defined(__GNUG__)
				//  for any reason gcc has a problem with bind(...)
				//static std::function<void(boost::beast::websocket::frame_type, boost::beast::string_view)> f;

				//
				//	This mess will be fixed with Boost 1.67
				//	see: https://github.com/boostorg/beast/issues/1054
				//
				ping_cb_ = std::bind(&websocket_session::on_control_callback,
						//this->shared_from_this(),
						this,
						std::placeholders::_1,
						std::placeholders::_2);
				ws_.control_callback(ping_cb_);

//#else
				//ws_.control_callback(std::bind(&websocket_session::on_control_callback
				//	, this->shared_from_this()
				//	//, this
				//	, std::placeholders::_1
				//	, std::placeholders::_2));
//#endif
				// Run the timer. The timer is operated
				// continuously, this simplifies the code.
				on_timer({});

				// Set the timer
				timer_.expires_after(std::chrono::seconds(15));

				//
				//	check subprotocols
				//
				const auto pcount = req.count(boost::beast::http::field::sec_websocket_protocol);
				if (pcount != 0)
				{
					CYNG_LOG_TRACE(logger_, pcount << " ws subprotocol(s) available");
					ws_.async_accept_ex(req, [&req, this](boost::beast::websocket::response_type& res) {

						res.set(boost::beast::http::field::user_agent, NODE::version_string);

						//
						//	accept all subprotocol
						//
						for (auto pos = req.find(boost::beast::http::field::sec_websocket_protocol); pos != req.end(); pos++)
						{
							CYNG_LOG_TRACE(logger_, pos->name() << ": " << pos->value());
							res.set(boost::beast::http::field::sec_websocket_protocol, pos->value());
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

			/**
			 * Execute a program
			 */
			void run(cyng::vector_t&& prg);

		private:
			/**
			 * Called to indicate activity from the remote peer
			 */
			void activity();

			/**
			 * Called when the timer expires.
			 */
			void on_timer(boost::system::error_code ec);

			void ws_send_json(cyng::context& ctx);

		private:
			boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			boost::asio::steady_timer timer_;
			boost::beast::multi_buffer buffer_;
			cyng::logging::log_ptr logger_;
			connection_manager& connection_manager_;
			bus::shared_type bus_;
			cyng::controller vm_;	//!< websocket
			char ping_state_ = 0;
			std::function<void(boost::beast::websocket::frame_type, boost::beast::string_view)>	ping_cb_;
		};

		cyng::object make_websocket(cyng::logging::log_ptr
			, connection_manager& cm
			, boost::asio::ip::tcp::socket&& socket
			, bus::shared_type
			, boost::uuids::uuid);
	}
}

#include <cyng/intrinsics/traits.hpp>

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
			constexpr static char name[] = "ws";
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

}
#endif
