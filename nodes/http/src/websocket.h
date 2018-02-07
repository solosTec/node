/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_HTTP_WEBSOCKET_H
#define NODE_HTTP_WEBSOCKET_H

#include <NODE_project_info.h>
#include "mail_config.h"
#include <cyng/compatibility/io_service.h>
// #include <cyng/compatibility/general.h>
#include <boost/beast/websocket.hpp>
#include <boost/asio/steady_timer.hpp>
#include <memory>
#include <cyng/log.h>

namespace node 
{
	class connection_manager;
	class websocket_session : public std::enable_shared_from_this<websocket_session>
	{
		cyng::logging::log_ptr logger_;
		connection_manager& connection_manager_;
		boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
		boost::asio::strand<boost::asio::io_context::executor_type> strand_;
		boost::asio::steady_timer timer_;
		boost::beast::multi_buffer buffer_;
		char ping_state_;
		const mail_config mail_config_;


	public:
		// Take ownership of the socket
		explicit
		websocket_session(cyng::logging::log_ptr logger
		, connection_manager& cm
		, boost::asio::ip::tcp::socket socket
		, mail_config const&);

		// Start the asynchronous operation
		template<class Body, class Allocator>
		void do_accept(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req)
		{
			
			// Set the control callback. This will be called
			// on every incoming ping, pong, and close frame.
            
            //
            //  for any reason gcc prior 5.4 has a problem with bind(...)
			//	and actual version 7.2.0 has problems too
            //
<<<<<<< .merge_file_a04560
#if defined(NODE_LEGACY_MODE_ON) || defined(__GNUC__) || defined(__GNUG__)
=======
//#if defined(NODE_LEGACY_MODE_ON)
>>>>>>> .merge_file_a14300
            std::function<void(boost::beast::websocket::frame_type, boost::beast::string_view)> f = std::bind(
 					&websocket_session::on_control_callback,
 					this,
 					std::placeholders::_1,
 					std::placeholders::_2);
			ws_.control_callback(f);
<<<<<<< .merge_file_a04560
#else				
  			ws_.control_callback(
  				std::bind(
  					&websocket_session::on_control_callback,
  					this,
  					std::placeholders::_1,
  					std::placeholders::_2));
#endif		
=======
// #else				
//  			ws_.control_callback(
//  				std::bind(
//  					&websocket_session::on_control_callback,
//  					this,
//  					std::placeholders::_1,
//  					std::placeholders::_2));
// #endif		
>>>>>>> .merge_file_a14300
			// Run the timer. The timer is operated
			// continuously, this simplifies the code.
			on_timer({});

			// Set the timer
			timer_.expires_after(std::chrono::seconds(15));
			
			bool cyng_protocol = false;
			const auto pcount = req.count(boost::beast::http::field::sec_websocket_protocol);
			if (pcount != 0)
			{
// 				CYNG_LOG_INFO(logger_, "websocket::do_accept - " << protocols << " available");
				for (auto pos = req.find(boost::beast::http::field::sec_websocket_protocol); pos != req.end(); pos++)
				{
 					CYNG_LOG_INFO(logger_, pos->name() << ": " << pos->value());
					cyng_protocol = pos->value().find("CYNG") != std::string::npos;
				}
			}
			
			//
			//	debug
			//	print all headers
// 			for (auto pos = req.begin(); pos != req.end(); pos++)
// 			{
// 				std::cout << pos->name() << ": " << pos->value() << std::endl;
// 			}
			
			// Accept the websocket handshake
			ws_.async_accept_ex(req, [cyng_protocol](boost::beast::websocket::response_type& res){
					res.set(boost::beast::http::field::user_agent, NODE::version_string);
					if (cyng_protocol)
					{
						res.set(boost::beast::http::field::sec_websocket_protocol, "CYNG");
					}
				},
				boost::asio::bind_executor(
					strand_,
					std::bind(
						&websocket_session::on_accept,
						shared_from_this(),
						std::placeholders::_1)));
// 			ws_.async_accept(
// 				req,
// 				boost::asio::bind_executor(
// 					strand_,
// 					std::bind(
// 						&websocket_session::on_accept,
// 						shared_from_this(),
// 						std::placeholders::_1)));
		}

		// Called when the timer expires.
		void on_timer(boost::system::error_code ec);

		void on_accept(boost::system::error_code ec);

		void do_read();

		void on_read(boost::system::error_code ec,
			std::size_t bytes_transferred);

		void on_write(boost::system::error_code ec,
			std::size_t bytes_transferred);
		
		// Called to indicate activity from the remote peer
		void activity();
		
		// Called after a ping is sent.
		void on_ping(boost::system::error_code ec);
		
		void on_control_callback(boost::beast::websocket::frame_type kind,
			boost::beast::string_view payload);
		
		void do_close();
		
	private:
		void start_timer(std::int64_t);
		
	};	
	
	/**
	 * Websockets are managed by the session manager as 
	 * shared pointers 
	 */
	using wss_ptr = std::shared_ptr<websocket_session>;
	
}

#endif


