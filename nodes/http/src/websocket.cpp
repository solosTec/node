/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "websocket.h"
#include "connections.h"
#include <cyng/json.h>
#include <cyng/io/serializer.h>
#include <cyng/dom/reader.h>
#include <cyng/value_cast.hpp>
#include <cyng/factory.h>
#include <cyng/sys/memory.h>
#include <cyng/sys/cpu.h>
#include <cyng/io/mail/message.hpp>
#include <cyng/io/mail/smtp.hpp>
#include <cyng/io/io_chrono.hpp>

namespace node
{

	websocket_session::websocket_session(cyng::logging::log_ptr logger
		, connection_manager& cm
		, boost::asio::ip::tcp::socket socket
		, mail_config const& mc)
	: logger_(logger)
		, connection_manager_(cm)
		, ws_(std::move(socket))
		, strand_(ws_.get_executor())
		, timer_(ws_.get_executor().context(), (std::chrono::steady_clock::time_point::max)())
		, ping_state_(0)
		, mail_config_(mc)
    { }


    // Called when the timer expires.
    void websocket_session::on_timer(boost::system::error_code ec)
    {
        if(ec && ec != boost::asio::error::operation_aborted)
		{
			CYNG_LOG_WARNING(logger_, "websocket::on_timer - aborted");
//             return fail(ec, "timer");
			return;
		}

        // See if the timer really expired since the deadline may have moved.
        if(timer_.expiry() <= std::chrono::steady_clock::now())
        {
            // If this is the first time the timer expired,
            // send a ping to see if the other end is there.
            if(ws_.is_open() && ping_state_ == 0)
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
                            shared_from_this(),
                            std::placeholders::_1)));
            }
            else
            {
                // The timer expired while trying to handshake,
                // or we sent a ping and it never completed or
                // we never got back a control frame, so close.

                // Closing the socket cancels all outstanding operations. They
                // will complete with boost::asio::error::operation_aborted
				connection_manager_.stop(shared_from_this());
//                 ws_.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
//                 ws_.next_layer().close(ec);
                return;
            }
        }

        // Wait on the timer
        timer_.async_wait(
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &websocket_session::on_timer,
                    shared_from_this(),
                    std::placeholders::_1)));
	}

    void websocket_session::on_accept(boost::system::error_code ec)
    {
        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
		{
			CYNG_LOG_WARNING(logger_, "websocket::on_accept - aborted");
			return;
		}

        if(ec)
		{
			CYNG_LOG_ERROR(logger_, "websocket::on_accept: " << ec);
			return;
//             return fail(ec, "accept");
		}

		CYNG_LOG_TRACE(logger_, "websocket::on_accept - OK");
// 		CYNG_LOG_TRACE(logger_, "websocket::on_accept( " << boost::beast::buffers(buffer_.data()) << " )");		
		
        // Read a message
        do_read();
    }

    void websocket_session::do_read()
    {
        // Set the timer - cancel the running timer
//         timer_.expires_after(std::chrono::seconds(15));

        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &websocket_session::on_read,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
    }

    void websocket_session::on_read(boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);
		CYNG_LOG_TRACE(logger_, "websocket::on_read( " << bytes_transferred << " bytes )");

        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
		{
			CYNG_LOG_WARNING(logger_, "websocket::on_read - aborted");
            return;
		}

        // This indicates that the websocket_session was closed
        if(ec == boost::beast::websocket::error::closed)
		{
			CYNG_LOG_ERROR(logger_, "websocket::on_read: "<< ec << ": " << ec.message());
// 			CYNG_LOG_ERROR(logger_, "websocket::on_read - closed");
// 			connection_manager_.stop(shared_from_this());
            return;
		}

        if(ec)
		{
			CYNG_LOG_ERROR(logger_, "websocket::on_read: "<< ec << ": " << ec.message());
//             fail(ec, "read");
			return;
		}

        // Note that there is activity
        activity();

		
        // Echo the message
//         {"cmd":"subscribe","channel":"sys.cpu.load","timer":false}
		std::stringstream msg; 
		msg << boost::beast::buffers(buffer_.data());
		const std::string str = msg.str();
		CYNG_LOG_TRACE(logger_, "websocket::on_read - " << str);
// 		std::cout << msg.str() << std::endl;
		
		const auto obj = cyng::json::read(str);
// 		cyng::io::serialize_json(std::cout, obj);
// 		std::cout << std::endl;
		auto reader = cyng::make_reader(obj);

		//	{"cmd": "subscribe", "channel": "sys.cpu.load", "timer": true}
		
		bool sys_cpu_load = false;
// 		bool io_mail_out = false;
		if (cyng::value_cast<std::string>(reader.get("cmd"), "") == "update")
		{
			CYNG_LOG_INFO(logger_, "subscribe " << cyng::value_cast<std::string>(reader.get("channel"), ""));
			sys_cpu_load = cyng::value_cast<std::string>(reader.get("channel"), "") == "sys.cpu.usage.total";
		}
		
// 		{"cmd":"send","channel":"io.mail.out","contact":{"name":"Sylko Olzscher","email":"CmdPirx@gmail.com"}}
		else if (cyng::value_cast<std::string>(reader.get("cmd"), "") == "send")
		{
			//	{("name":Sylko Olzscher),("email":CmdPirx@gmail.com),("msg":the message)}
			CYNG_LOG_INFO(logger_, "send " << cyng::io::to_str(reader.get("contact")));
// 			io_mail_out = cyng::value_cast<std::string>(reader.get("channel"), "") == "io.mail.out";
			

			const std::string customer = cyng::io::to_str(reader["contact"].get("name"));
			const std::string address = cyng::io::to_str(reader["contact"].get("email"));
			const std::string msg = cyng::io::to_str(reader["contact"].get("msg"));
			
			/*
			 * the works but not on GCP so I have it comment out 
			 */ 
			mailio::message message;
			message.sender(mailio::mail_address(mail_config_.sender_.name_, mail_config_.sender_.address_)); // set the correct sender name and address
			for (auto rec : mail_config_.recipients_)
			{
				// set the correct recipent name and address
				message.add_recipient(mailio::mail_address(rec.name_, rec.address_));
			}
			message.subject("contact request [" + address + "]");
			
			//
			//	enable stream operator for timepoint
			//
			using cyng::operator<<;	
			std::stringstream ss;
			ss 
			<< "origin: "
			<< ws_.lowest_layer().remote_endpoint()
			<< std::endl
			<< "time: "
			<< std::chrono::system_clock::now()
			<< " UTC"
			<< std::endl
			;
			message.attach(ss, "meta.txt", mailio::message::media_type_t::TEXT, "plain");
			message.content(msg);
		
			// connect to server
            CYNG_LOG_TRACE(logger_, "connect to mail server " << mail_config_.host_ << ':' << mail_config_.port_);
			mailio::smtps conn(mail_config_.host_, mail_config_.port_);	// TLS
			// modify username/password to use real credentials
			conn.authenticate(mail_config_.auth_name_, mail_config_.auth_pwd_, mailio::smtps::auth_method_t::START_TLS);
			conn.submit(message);
			
		}
		else if (cyng::value_cast<std::string>(reader.get("cmd"), "") == "store")
		{
			//	{("name":Sylko Olzscher),("email":CmdPirx@gmail.com),("msg":the message)}
			CYNG_LOG_INFO(logger_, "store " << cyng::io::to_str(reader.get("contact")));
// 			io_mail_out = cyng::value_cast<std::string>(reader.get("channel"), "") == "io.mail.out";
			

			const std::string customer = cyng::io::to_str(reader["contact"].get("name"));
			const std::string address = cyng::io::to_str(reader["contact"].get("email"));
			const std::string msg = cyng::io::to_str(reader["contact"].get("msg"));
			
			std::fstream fs("/var/mail/mail_" + customer + ".txt", std::fstream::out | std::fstream::app);
			if (fs.is_open())
			{
				using cyng::operator<<;	
				fs 
				<< "origin: "
				<< ws_.lowest_layer().remote_endpoint()
				<< std::endl
				<< "time: "
				<< std::chrono::system_clock::now()
				<< " UTC"
				<< std::endl
				<< "customer: "
				<< customer
				<< std::endl
				<< "address: "
				<< address
				<< std::endl
				<< "msg: "
				<< msg
				<< std::endl
				;
				
			}
		}
		
		//
		//	generate payload
		//
		std::stringstream ss;
		if (sys_cpu_load)
		{
			const double usage = cyng::sys::get_total_cpu_load();
			CYNG_LOG_TRACE(logger_, "get_total_cpu_load( " << usage << " )");
			auto res = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("update")),
				cyng::param_factory("channel", std::string("sys.cpu.usage.total")),
				cyng::param_factory("value", usage));
// 				cyng::param_factory("value", cyng::sys::get_cpu_load_by_process()));
			cyng::json::write(ss, cyng::make_object(res));
			
			// Set the timer
// 			start_timer();
			
		}
		else 
		{
			auto res = cyng::tuple_factory(
				cyng::param_factory("cmd", std::string("update")),
				cyng::param_factory("key", std::string("sys.time.local")),
				cyng::param_factory("value", std::time(nullptr)));
			cyng::json::write(ss, cyng::make_object(res));
		}
		
// 		std::cout << boost::beast::buffers(buffer_.data()) << std::endl;
// 		CYNG_LOG_TRACE(logger_, "websocket::on_read( " << boost::beast::buffers(buffer_.data()) << " )");

		//
		//	clear buffer
		//
		ws_.text();
        ws_.text(ws_.got_text());
		
// 		buffer_.consume(buffer_.size());

		reply_ = ss.str();
        ws_.async_write(
			boost::asio::buffer(reply_),
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &websocket_session::on_write,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
    }

    void websocket_session::on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // Happens when the timer closes the socket
        if(ec == boost::asio::error::operation_aborted)
		{
			CYNG_LOG_WARNING(logger_, "websocket::on_write - aborted");
            return;
		}

        if(ec)
		{
			CYNG_LOG_ERROR(logger_, "websocket::on_write: " << ec);
//             return fail(ec, "write");
			return;
		}

        // Clear the buffer
        buffer_.consume(buffer_.size());
		
		CYNG_LOG_TRACE(logger_, "websocket::on_write( " << bytes_transferred << " bytes )");

        // Do another read
        do_read();
    }
    
	void websocket_session::start_timer(std::int64_t n)
	{
		CYNG_LOG_TRACE(logger_, "websocket - start timer");
		
		//	start timer
		timer_.expires_from_now(std::chrono::seconds(n));
		
		// Wait on the timer asynchronously
		timer_.async_wait(
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&websocket_session::on_timer,
					shared_from_this(),
					std::placeholders::_1)));					

	}
    
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
        if(ec == boost::asio::error::operation_aborted)
		{
			CYNG_LOG_WARNING(logger_, "on_ping - aborted");
            return;
		}

        if(ec)
		{
			CYNG_LOG_ERROR(logger_, "on_ping - failed " << ec << ": " << ec.message());
//             return fail(ec, "ping");
			return;
		}

		CYNG_LOG_TRACE(logger_, "on_ping - state: " << +ping_state_);
        // Note that the ping was sent.
        if(ping_state_ == 1)
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
		
		switch(kind)
		{
			/// A close frame was received
			case boost::beast::websocket::frame_type::close:
				CYNG_LOG_TRACE(logger_, "ws::close - " << payload);
				break;

			/// A ping frame was received
			case boost::beast::websocket::frame_type::ping:
				CYNG_LOG_TRACE(logger_, "ws::ping - " << payload);
				break;

			/// A pong frame was received
			case boost::beast::websocket::frame_type::pong:
				CYNG_LOG_TRACE(logger_, "ws::pong - " << payload);
				break;
				
			default:
				break;
		}

    
		
        // Note that there is activity
        activity();
    }    
    
	void websocket_session::do_close()
	{
		CYNG_LOG_TRACE(logger_, "ws::do_close(cancel timer)");
		timer_.cancel();

		CYNG_LOG_TRACE(logger_, "ws::do_close(shutdown " << shared_from_this().use_count() << " )");
		
		// Send a TCP shutdown
		boost::system::error_code ec;
		ws_.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		ws_.next_layer().close(ec);
		
		if (ec)
		{
			CYNG_LOG_WARNING(logger_, "ws::do_close("<< ec << ")");
		}

		// At this point the connection is closed gracefully
	}
    
};


