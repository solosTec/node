/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_HTTP_SESSION_H
#define SMF_HTTP_SESSION_H

#include <smf/http/auth.h>

#include <cyng/log/logger.h>

#include <type_traits>
#include <vector>
#include <memory>

#include <boost/assert.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>


namespace smf {
	namespace http
	{
		class session : public std::enable_shared_from_this<session>
		{
            // This queue is used for HTTP pipelining.
            class queue
            {
                // Maximum number of responses we will queue
                using limit = std::integral_constant< std::uint32_t, 8>;

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

                /**
                 * @return true if we have reached the queue limit
                 */
                bool is_full() const;

                /**
                 * Called when a message finishes sending
                 * @return true if the caller should initiate a read
                 */
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

                        work_impl(session& self, boost::beast::http::message<isRequest, Body, Fields>&& msg)
                        : self_(self)
                            , msg_(std::move(msg))
                        {}

                        void operator()()
                        {
                            boost::beast::http::async_write(
                                self_.stream_,
                                msg_,
                                boost::beast::bind_front_handler(
                                    &session::on_write,
                                    self_.shared_from_this(),
                                    msg_.need_eof()));
                        }
                    };

                    // Allocate and store the work
                    items_.push_back( boost::make_unique<work_impl>(self_, std::move(msg)));

                    // If there was no previous work, start this one
                    if (items_.size() == 1) {
                        (*items_.front())();
                    }
                }
            };

		public:
            //  callback for websockets
            using ws_cb = std::function<void(boost::asio::ip::tcp::socket&&, boost::beast::http::request<boost::beast::http::string_body>)>;
            using post_cb = std::function<std::filesystem::path(std::string, std::string, std::string)>;

			session(boost::asio::ip::tcp::socket&& socket
                , cyng::logger
                , std::string doc_root
                , std::map<std::string, std::string> const& redirects_intrinsic
                , auth_dirs const& auths_
                , std::uint64_t const max_upload_size
                , std::string const nickname
                , std::chrono::seconds const timeout
                , ws_cb
                , post_cb
            );

            /**
             * start session
             */
            void run();

        private:
            void do_read();
            void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
            void on_write(bool close, boost::beast::error_code ec, std::size_t bytes_transferred);
            void do_close();
            void handle_request(boost::beast::http::request<boost::beast::http::string_body>&&);
            void handle_get_head_request(boost::beast::http::request<boost::beast::http::string_body>&&, std::string const&, bool);
            //void handle_head_request(boost::beast::http::request<boost::beast::http::string_body>&&, std::string const&);
            void handle_post_request(boost::beast::http::request<boost::beast::http::string_body>&&, std::string const&);
            void handle_options_request(boost::beast::http::request<boost::beast::http::string_body>&& req);

            bool check_auth(boost::beast::http::request<boost::beast::http::string_body> const& req);
            bool check_auth(boost::beast::http::request<boost::beast::http::string_body> const& req, boost::beast::string_view);

            /**
             * sanitize URL/target
             */
            void intrinsic_redirect(std::string&);

            boost::beast::http::response<boost::beast::http::string_body> send_bad_request(std::uint32_t version
                , bool keep_alive
                , std::string const& why);

            boost::beast::http::response<boost::beast::http::string_body> send_not_found(std::uint32_t version
                , bool keep_alive
                , std::string target);

            boost::beast::http::response<boost::beast::http::string_body> send_server_error(std::uint32_t version
                , bool keep_alive
                , boost::system::error_code ec);

            boost::beast::http::response<boost::beast::http::string_body> send_redirect(std::uint32_t version
                , bool
                , std::string host
                , std::string target);

            boost::beast::http::response<boost::beast::http::empty_body> send_head(std::uint32_t version
                , bool
                , std::string const& path
                , std::uint64_t);

            boost::beast::http::response<boost::beast::http::file_body> send_get(std::uint32_t version
                , bool
                , boost::beast::http::file_body::value_type&&
                , std::string const& path
                , std::uint64_t size);        

            boost::beast::http::response<boost::beast::http::string_body> send_204(std::uint32_t version
                , bool
                , std::string options);

            boost::beast::http::response<boost::beast::http::string_body> send_not_authorized(std::uint32_t version
                , bool keep_alive
                , std::string target
                , std::string realm);

            void start_download(std::filesystem::path, std::filesystem::path);

		private:
            boost::beast::tcp_stream stream_;
            boost::beast::flat_buffer buffer_;
            cyng::logger logger_;
            std::string const doc_root_;
            std::map<std::string, std::string> const& redirects_intrinsic_;
            auth_dirs const& auths_;
            std::uint64_t const max_upload_size_;
            std::string const nickname_;
            std::chrono::seconds const timeout_;
            ws_cb ws_cb_;
            post_cb post_cb_;
            queue queue_;

            // The parser is stored in an optional container so we can
            // construct it from scratch it at the beginning of each new message.
            boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> parser_;
		};

	}	//	http
}

#endif
