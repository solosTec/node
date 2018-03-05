/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTP_SRV_SESSION_H
#define NODE_LIB_HTTP_SRV_SESSION_H

#include <cyng/log.h>
#include <cyng/store/db.h>
#include <smf/cluster/bus.h>
//#include <cyng/vm/controller.h>
#include <memory>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>

namespace node
{
	namespace http
	{
		class connection_manager;
		class session : public std::enable_shared_from_this<session>
		{
			friend class connection_manager;

			// This queue is used for HTTP pipelining.
			class queue
			{
				enum
				{
					// Maximum number of responses we will queue
					limit = 8
				};

				// The type-erased, saved work item
				struct work
				{
					virtual ~work() = default;
					virtual void operator()() = 0;
				};

				session& self_;
				std::vector<std::unique_ptr<work>> items_;

			public:
				explicit queue(session& self);

				// Returns `true` if we have reached the queue limit
				bool is_full() const;

				// Called when a message finishes sending
				// Returns `true` if the caller should initiate a read
				bool on_write();

				// Called by the HTTP handler to send a response.
				template<bool isRequest, class Body, class Fields>
				void operator()(boost::beast::http::message<isRequest, Body, Fields>&& msg)
				{
					// This holds a work item
					struct work_impl : work
					{
						session& self_;
						boost::beast::http::message<isRequest, Body, Fields> msg_;

						work_impl(session& self,
							boost::beast::http::message<isRequest, Body, Fields>&& msg)
							: self_(self)
							, msg_(std::move(msg))
						{
						}

						void operator()()
						{
							boost::beast::http::async_write(
								self_.socket_,
								msg_,
								boost::asio::bind_executor(
									self_.strand_,
									std::bind(
										&session::on_write,
										self_.shared_from_this(),
										std::placeholders::_1,
										msg_.need_eof())));
						}
					};

					// Allocate and store the work
					items_.emplace_back(new work_impl(self_, std::move(msg)));

					// If there was no previous work, start this one
					if (items_.size() == 1)
					{
						(*items_.front())();
					}
				}
			};


		public:
			// Take ownership of the socket
			explicit session(cyng::logging::log_ptr
				, connection_manager& cm
				, boost::asio::ip::tcp::socket socket
				, std::string const& doc_root
				, node::bus::shared_type
				, cyng::store::db&
				, boost::uuids::uuid);
			virtual ~session();

			// Start the asynchronous operation
			void run();
			void do_read();

			// Called when the timer expires.
			void on_timer(boost::system::error_code ec);

			void on_read(boost::system::error_code ec);
			void on_write(boost::system::error_code ec, bool close);
			void do_close();

		private:
			boost::asio::ip::tcp::socket socket_;
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			boost::asio::steady_timer timer_;
			boost::beast::flat_buffer buffer_;
			cyng::logging::log_ptr logger_;
			connection_manager& connection_manager_;
			std::string const& doc_root_;
			bus::shared_type bus_;
			const boost::uuids::uuid tag_;
			cyng::store::db& cache_;
			//cyng::controller vm_;	//!< http session
			boost::beast::http::request<boost::beast::http::string_body> req_;
			queue queue_;
		};

		/**
		* sessions are managed by the session manager as
		* shared pointers
		*/
		using session_ptr = std::shared_ptr<session>;
	}
}

#endif
