/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <influxdb.h>
#include <smf.h>
#include <smf/mbus/server_id.h>

#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/tag.hpp>
#include <cyng/obj/value_cast.hpp>

#include <boost/beast.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

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

        std::string push_over_http(
            boost::asio::io_context &ctx,
            std::string const &host,
            std::string const &service,
            std::string const &protocol,
            std::string const &cert,
            std::string const &db,
            std::string const &stmt) {
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
                    boost::beast::http::verb::post, "/write?db=" + db, 11};
                req.set(boost::beast::http::field::host, host);
                req.set(boost::beast::http::field::user_agent, SMF_VERSION_SUFFIX);

                req.body() = stmt;
                req.prepare_payload();

                //
                // Send the HTTP request to the remote host
                //
                boost::beast::http::write(stream, req);

                // This buffer is used for reading and must be persisted
                boost::beast::flat_buffer buffer;

                // Declare a container to hold the response
                boost::beast::http::response<boost::beast::http::string_body> res;
                // boost::beast::http::response<boost::beast::http::dynamic_body> res;

                // Receive the HTTP response
                boost::beast::http::read(stream, buffer, res);

                // Gracefully close the stream
                // stream.close();

                return res.body();

            } catch (boost::system::system_error const &ex) {
                std::cout << "***warning : resolve " << host << ':' << service << "failed: " << ex.what() << std::endl;
            }

            return "failed";
        }

        std::string to_line_protocol(cyng::object obj) {
            switch (obj.rtti().tag()) {
            case cyng::TC_STRING:
                return to_line_protocol(cyng::io::to_plain(obj));
            case cyng::TC_INT8:
            case cyng::TC_INT16:
            case cyng::TC_INT32:
            case cyng::TC_INT64:
                return cyng::io::to_plain(obj) + "i";
            case cyng::TC_UINT8:
            case cyng::TC_UINT16:
            case cyng::TC_UINT32:
            case cyng::TC_UINT64:
                return std::to_string(cyng::numeric_cast<std::uint64_t>(obj, 0u)) + "i";
            case cyng::TC_TIME_POINT:
                return to_line_protocol(cyng::value_cast(obj, std::chrono::system_clock::now()));
            case cyng::TC_NULL:
                return "0.0";
            default:
                break;
            }
            return "\"" + cyng::io::to_plain(obj) + "\"";
        }

        std::string to_line_protocol(std::string s) {

            if (s.empty())
                return "\"\"";

            //
            //	hex value
            //	test for [0..9a..f]
            //
            //
            if (std::all_of(s.begin(), s.end(), [](char c) {
                    switch (c) {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case 'a':
                    case 'b':
                    case 'c':
                    case 'd':
                    case 'e':
                    case 'f':
                    case 'A':
                    case 'B':
                    case 'C':
                    case 'E':
                    case 'D':
                    case 'F':
                        return true;
                    default:
                        break;
                    }
                    return false;
                })) {

                //
                //	hex value
                //
                try {
                    auto const u = std::stoull(s, 0, 16);
                    return std::to_string(u);
                } catch (std::exception const &ex) {
                    boost::ignore_unused(ex);
                    //	continue
                }
            }

            //
            //	float/double value
            //	test for [0..9] and '.'
            //
            //
            if (std::all_of(s.begin(), s.end(), [](char c) {
                    switch (c) {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '.':
                        return true;
                    default:
                        break;
                    }
                    return false;
                })) {

                //
                //	float/double value
                //

                return s;
            }

            return "\"" + s + "\"";
        }
        std::string to_line_protocol(std::chrono::system_clock::time_point tp) {
            auto const s = std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count());
            return (s.size()) > 20 ? s.substr(0, 20) : s;
        }

    } // namespace influx
} // namespace smf
