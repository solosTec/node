/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/https/srv/server.h>
#include <smf/https/srv/session.h>

namespace node
{
	namespace https
	{
		server::server(boost::asio::io_context& ioc,
			boost::asio::ssl::context& ctx,
			boost::asio::ip::tcp::endpoint endpoint,
			std::string const& doc_root)
		: ctx_(ctx)
			, acceptor_(ioc)
			, socket_(ioc)
			, doc_root_(doc_root)
			, is_listening_(false)
		{
			boost::system::error_code ec;

			// Open the acceptor
			acceptor_.open(endpoint.protocol(), ec);
			if (ec)
			{
				BOOST_ASSERT_MSG(!ec, "OPEN");
				return;
			}

			// Allow address reuse
			acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
			if (ec)
			{
				BOOST_ASSERT_MSG(!ec, "SET_OPTION");
				return;
			}

			// Bind to the server address
			acceptor_.bind(endpoint, ec);
			if (ec)
			{
				BOOST_ASSERT_MSG(!ec, "BIND");
				return;
			}

			// Start listening for connections
			acceptor_.listen(
				boost::asio::socket_base::max_listen_connections, ec);
			if (ec)
			{
				BOOST_ASSERT_MSG(!ec, "LISTEN");
				return;
			}
		}

		// Start accepting incoming connections
		bool server::run()
		{
			//
			//	set listener flag immediately to prevent
			//	a concurrent call of this same
			//	method
			//
			if (acceptor_.is_open() && !is_listening_.exchange(true))
			{
				do_accept();
			}
			return is_listening_;
		}

		void server::do_accept()
		{
			acceptor_.async_accept(
				socket_,
				std::bind(
					&server::on_accept,
					shared_from_this(),
					std::placeholders::_1));
		}

		void server::on_accept(boost::system::error_code ec)
		{
			if (ec)
			{
				BOOST_ASSERT_MSG(!ec, "ACCEPT");
			}
			else
			{
				// Create the session and run it
				std::make_shared<session>(
					std::move(socket_),
					ctx_,
					doc_root_)->run();
			}

			// Accept another connection
			do_accept();
		}

		void server::close()
		{
			if (is_listening_)
			{
				//cyng::async::unique_lock<cyng::async::mutex> lock(mutex_);

				acceptor_.cancel();
				acceptor_.close();

				//CYNG_LOG_TRACE(logger_, "listener is waiting for cancellation");
				//	wait for cancellation of accept operation
				//shutdown_complete_.wait(lock, [this] {
				//	return !is_listening_.load();
				//});
				//CYNG_LOG_TRACE(logger_, "listener cancellation complete");
			}
			else
			{
				//CYNG_LOG_WARNING(logger_, "listener already closed");
			}

			//
			//	stop all connections
			//
			//CYNG_LOG_INFO(logger_, "stop all connections");
			//connection_manager_.stop_all();

		}

	}
}

//------------------------------------------------------------------------------

//int main(int argc, char* argv[])
//{
//	// Check command line arguments.
//	if (argc != 5)
//	{
//		std::cerr <<
//			"Usage: http-server-async-ssl <address> <port> <doc_root> <threads>\n" <<
//			"Example:\n" <<
//			"    http-server-async-ssl 0.0.0.0 8080 . 1\n";
//		return EXIT_FAILURE;
//	}
//	auto const address = boost::asio::ip::make_address(argv[1]);
//	auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
//	std::string const doc_root = argv[3];
//	auto const threads = std::max<int>(1, std::atoi(argv[4]));
//
//	// The io_context is required for all I/O
//	boost::asio::io_context ioc{ threads };
//
//	// The SSL context is required, and holds certificates
//	ssl::context ctx{ ssl::context::sslv23 };
//
//	// This holds the self-signed certificate used by the server
//	load_server_certificate(ctx);
//
//	// Create and launch a listening port
//	std::make_shared<server>(
//		ioc,
//		ctx,
//		tcp::endpoint{ address, port },
//		doc_root)->run();
//
//	// Run the I/O service on the requested number of threads
//	std::vector<std::thread> v;
//	v.reserve(threads - 1);
//	for (auto i = threads - 1; i > 0; --i)
//		v.emplace_back(
//			[&ioc]
//	{
//		ioc.run();
//	});
//	ioc.run();
//
//	return EXIT_SUCCESS;
//}