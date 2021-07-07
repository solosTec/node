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

#include <boost/algorithm/string.hpp>
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

        std::string get_area_name(std::string const &id) {

            if (boost::algorithm::equals(id, "35074612") || boost::algorithm::equals(id, "35074769") ||
                boost::algorithm::equals(id, "35074759") || boost::algorithm::equals(id, "35074669") ||
                boost::algorithm::equals(id, "35074664") || boost::algorithm::equals(id, "35074665") ||
                boost::algorithm::equals(id, "35074742") || boost::algorithm::equals(id, "35074745") ||
                boost::algorithm::equals(id, "35074728") || boost::algorithm::equals(id, "35074785") ||
                boost::algorithm::equals(id, "35074743") || boost::algorithm::equals(id, "35074631") ||
                boost::algorithm::equals(id, "35074613") || boost::algorithm::equals(id, "35074647") ||
                boost::algorithm::equals(id, "35074622") || boost::algorithm::equals(id, "35074648") ||
                boost::algorithm::equals(id, "35074731") || boost::algorithm::equals(id, "35074615") ||
                boost::algorithm::equals(id, "35074655") || boost::algorithm::equals(id, "35074663") ||
                boost::algorithm::equals(id, "35074712") || boost::algorithm::equals(id, "35074786") ||
                boost::algorithm::equals(id, "35074770") || boost::algorithm::equals(id, "35074653") ||
                boost::algorithm::equals(id, "35074692") || boost::algorithm::equals(id, "35074787") ||
                boost::algorithm::equals(id, "35074676") || boost::algorithm::equals(id, "35074753") ||
                boost::algorithm::equals(id, "35074754") || boost::algorithm::equals(id, "35074715") ||
                boost::algorithm::equals(id, "35074701") || boost::algorithm::equals(id, "35074677") ||
                boost::algorithm::equals(id, "35074694") || boost::algorithm::equals(id, "35074788") ||
                boost::algorithm::equals(id, "35074691") || boost::algorithm::equals(id, "35074698") ||
                boost::algorithm::equals(id, "35074750") || boost::algorithm::equals(id, "35074710") ||
                boost::algorithm::equals(id, "35074708") || boost::algorithm::equals(id, "35074740") ||
                boost::algorithm::equals(id, "04354752") || boost::algorithm::equals(id, "04354755") ||
                boost::algorithm::equals(id, "04354754") || boost::algorithm::equals(id, "35074637") ||
                boost::algorithm::equals(id, "10320043") || boost::algorithm::equals(id, "10320047") ||
                boost::algorithm::equals(id, "10320044") || boost::algorithm::equals(id, "04354750") ||
                boost::algorithm::equals(id, "04354751") || boost::algorithm::equals(id, "04354749") ||
                boost::algorithm::equals(id, "10320042") || boost::algorithm::equals(id, "35074766") ||
                boost::algorithm::equals(id, "35074619") || boost::algorithm::equals(id, "10320046") ||
                boost::algorithm::equals(id, "35074738") || boost::algorithm::equals(id, "35074709") ||
                boost::algorithm::equals(id, "35074725") || boost::algorithm::equals(id, "35074633") ||
                boost::algorithm::equals(id, "35074783") || boost::algorithm::equals(id, "35074762") ||
                boost::algorithm::equals(id, "35074658") || boost::algorithm::equals(id, "10320040") ||
                boost::algorithm::equals(id, "35074763") || boost::algorithm::equals(id, "35074662") ||
                boost::algorithm::equals(id, "10320064") || boost::algorithm::equals(id, "35074718") ||
                boost::algorithm::equals(id, "10320071") || boost::algorithm::equals(id, "35074776") ||
                boost::algorithm::equals(id, "35074746") || boost::algorithm::equals(id, "35074707") ||
                boost::algorithm::equals(id, "14818483") || boost::algorithm::equals(id, "14818485") ||
                boost::algorithm::equals(id, "04354747") || boost::algorithm::equals(id, "04354746") ||
                boost::algorithm::equals(id, "10320041") || boost::algorithm::equals(id, "10320045") ||
                boost::algorithm::equals(id, "04354753") || boost::algorithm::equals(id, "04354748")) {
                return "Moumnat";
            }

            if (boost::algorithm::equals(id, "03496219") || boost::algorithm::equals(id, "03496225") ||
                boost::algorithm::equals(id, "03496230") || boost::algorithm::equals(id, "03496234") ||
                boost::algorithm::equals(id, "03496233") || boost::algorithm::equals(id, "03496218") ||
                boost::algorithm::equals(id, "03496220") || boost::algorithm::equals(id, "03496222") ||
                boost::algorithm::equals(id, "03496226") || boost::algorithm::equals(id, "03496228") ||
                boost::algorithm::equals(id, "03496236") || boost::algorithm::equals(id, "03496227") ||
                boost::algorithm::equals(id, "03496232") || boost::algorithm::equals(id, "03496217") ||
                boost::algorithm::equals(id, "03496223")) {
                return "Lot Yaku";
            }

            return "Yasmina";
        }

    } // namespace influx
} // namespace smf
