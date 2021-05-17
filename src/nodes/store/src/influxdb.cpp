/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <influxdb.h>
#include <smf.h>

#include <boost/beast.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

//#include <iostream>
//#include <boost/program_options.hpp>
//#include <boost/predef.h>
namespace smf {
    namespace influx {

        int create_db(
            boost::asio::io_context &ctx,
            std::ostream &os,
            std::string const &host,
            std::string const &service,
            std::string const &protocol,
            std::string const &cert,
            std::string const &db) {

            os << "***info : " << host << ':' << service << "/query --data-urlencode \"q=CREATE DATABASE " << db << "\""
               << std::endl;

            //
            //	create stream
            //
            boost::beast::tcp_stream stream(ctx);

            //
            // Look up the domain name
            //
            boost::asio::ip::tcp::resolver resolver(ctx);
            auto const results = resolver.resolve(host, service);

            try {

                //
                // Make the connection on the IP address we get from a lookup
                //
                boost::beast::get_lowest_layer(stream).connect(results);

                //
                // Set up an HTTP POST request message HTTP 1.1
                //
                boost::beast::http::request<boost::beast::http::string_body> req{
                    boost::beast::http::verb::post, "/query?q=CREATE+DATABASE+" + db, 11};
                req.set(boost::beast::http::field::host, host);
                req.set(boost::beast::http::field::user_agent, SMF_VERSION_SUFFIX);
                // req.body() = ("q=CREATE+DATABASE+" + db);
                req.prepare_payload();

                //
                // Send the HTTP request to the remote host
                //
                boost::beast::http::write(stream, req);

                // This buffer is used for reading and must be persisted
                boost::beast::flat_buffer buffer;

                // Declare a container to hold the response
                boost::beast::http::response<boost::beast::http::dynamic_body> res;

                // Receive the HTTP response
                boost::beast::http::read(stream, buffer, res);

                // Write the message to standard out
                os << res << std::endl;

                return EXIT_SUCCESS;
            } catch (boost::system::system_error const &ex) {
                std::cout << "***warning : resolve " << host << ':' << service << "failed: " << ex.what() << std::endl;
            }
            return EXIT_FAILURE;
        }

        int show_db(
            boost::asio::io_context &ctx,
            std::ostream &os,
            std::string const &host,
            std::string const &service,
            std::string const &protocol,
            std::string const &cert) {

            std::cout << "***info : " << host << ':' << service << "/query --data-urlencode \"q=SHOW DATABASES\"" << std::endl;

            //
            //	create stream
            //
            boost::beast::tcp_stream stream(ctx);

            //
            // Look up the domain name
            //
            boost::asio::ip::tcp::resolver resolver(ctx);
            auto const results = resolver.resolve(host, service);

            try {

                //
                // Make the connection on the IP address we get from a lookup
                //
                boost::beast::get_lowest_layer(stream).connect(results);

                //
                // Set up an HTTP POST request message HTTP 1.1
                //
                boost::beast::http::request<boost::beast::http::string_body> req{
                    boost::beast::http::verb::get, "/query?q=SHOW+DATABASES", 11};
                req.set(boost::beast::http::field::host, host);
                req.set(boost::beast::http::field::user_agent, SMF_VERSION_SUFFIX);
                // req.body() = "--data-urlencode \"q=SHOW DATABASES\"";
                req.prepare_payload();

                //
                // Send the HTTP request to the remote host
                //
                boost::beast::http::write(stream, req);

                // This buffer is used for reading and must be persisted
                boost::beast::flat_buffer buffer;

                // Declare a container to hold the response
                // boost::beast::http::response<boost::beast::http::dynamic_body> res;

                // Receive the HTTP response
                boost::beast::http::response<boost::beast::http::string_body> res;
                boost::beast::http::read(stream, buffer, res);

                // Write the message to standard out
                // os << res << std::endl;
                os << res.body() << std::endl;
                // os << boost::beast::buffers_to_string(res.body.data()) << std::endl;

                // Gracefully close the stream
                stream.close();

                return EXIT_SUCCESS;
            } catch (boost::system::system_error const &ex) {
                std::cout << "***warning : resolve " << host << ':' << service << "failed: " << ex.what() << std::endl;
            }
            return EXIT_FAILURE;
        }

        int drop_db(
            boost::asio::io_context &ctx,
            std::ostream &os,
            std::string const &host,
            std::string const &service,
            std::string const &protocol,
            std::string const &cert,
            std::string const &db) {

            os << "***info : " << host << ':' << service << "/query --data-urlencode \"q=DROP DATABASE\" " << db << std::endl;

            //
            //	create stream
            //
            boost::beast::tcp_stream stream(ctx);

            //
            // Look up the domain name
            //
            boost::asio::ip::tcp::resolver resolver(ctx);
            auto const results = resolver.resolve(host, service);

            try {

                //
                // Make the connection on the IP address we get from a lookup
                //
                boost::beast::get_lowest_layer(stream).connect(results);

                //
                // Set up an HTTP POST request message HTTP 1.1
                //
                boost::beast::http::request<boost::beast::http::string_body> req{
                    boost::beast::http::verb::post, "/query?q=DROP+DATABASE+" + db, 11};
                req.set(boost::beast::http::field::host, host);
                req.set(boost::beast::http::field::user_agent, SMF_VERSION_SUFFIX);
                req.prepare_payload();

                //
                // Send the HTTP request to the remote host
                //
                boost::beast::http::write(stream, req);

                // This buffer is used for reading and must be persisted
                boost::beast::flat_buffer buffer;

                // Declare a container to hold the response
                boost::beast::http::response<boost::beast::http::dynamic_body> res;

                // Receive the HTTP response
                boost::beast::http::read(stream, buffer, res);

                // Write the message to standard out
                os << res << std::endl;

                return EXIT_SUCCESS;
            } catch (boost::system::system_error const &ex) {
                std::cout << "***warning : resolve " << host << ':' << service << "failed: " << ex.what() << std::endl;
            }
            return EXIT_FAILURE;
        }
    } // namespace influx
} // namespace smf
