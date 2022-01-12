/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_DASH_HTTP_SERVER_H
#define SMF_DASH_HTTP_SERVER_H

#include <db.h>
#include <download.h>
#include <upload.h>

#include <smf/cluster/bus.h>
#include <smf/http/auth.h>
#include <smf/http/server.h>

#include <cyng/io/serialize.h>
#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/eod.h>

#include <functional>
#include <memory>
#include <string>
#include <tuple>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>

namespace smf {

    //
    //	forward declarations
    //
    namespace http {
        class ws;
    }
    using ws_sptr = std::shared_ptr<http::ws>;
    using ws_wptr = std::weak_ptr<http::ws>;

    class http_server {
      public:
        using blocklist_type = std::vector<boost::asio::ip::address>;

      public:
        http_server(
            boost::asio::io_context &,
            bus &,
            cyng::logger,
            std::string const &document_root,
            db &,
            blocklist_type &&,
            std::map<std::string, std::string> &&,
            http::auth_dirs const &);
        ~http_server();

        void stop(cyng::eod);
        void listen(boost::asio::ip::tcp::endpoint);

        /**
         * notify to remove an element
         */
        void notify_remove(std::string channel, cyng::key_t, boost::uuids::uuid tag);

        /**
         * notify to clear a table
         */
        void notify_clear(std::string channel, boost::uuids::uuid tag);

        /**
         * notify to insert an element
         */
        void notify_insert(std::string channel, cyng::record &&, boost::uuids::uuid tag);

        /**
         * notify to update an element
         */
        void notify_update(std::string channel, cyng::key_t, cyng::param_t const &, boost::uuids::uuid tag);

        /**
         * notify change in table size
         */
        void notify_size(std::string channel, std::size_t size, boost::uuids::uuid tag);

      private:
        /**
         * incoming connection
         */
        void accept(boost::asio::ip::tcp::socket &&);

        /**
         * upgrade to webscoket
         */
        void upgrade(boost::asio::ip::tcp::socket socket, boost::beast::http::request<boost::beast::http::string_body> req);

        /**
         * post => start download
         */
        std::filesystem::path post(std::string, std::string, std::string);

        /**
         * received text/json data from the websocket
         */
        void on_msg(boost::uuids::uuid tag, std::string);

        bool response_subscribe_channel(ws_sptr, std::string const &, boost::uuids::uuid tag);
        bool response_subscribe_channel(ws_sptr, std::string const &, std::string const &);
        void response_update_channel(ws_sptr, std::string const &);
        void response_subscribe_channel_meterwMBus(ws_sptr, std::string const &, std::string const &);
        void response_subscribe_channel_meterIEC(ws_sptr, std::string const &, std::string const &);
        void response_subscribe_channel_gwIEC(ws_sptr wsp, std::string const &name, std::string const &table_name);
        void response_subscribe_channel_gateway(ws_sptr, std::string const &, std::string const &);
        void response_subscribe_channel_meter(ws_sptr, std::string const &, std::string const &);

        void modify_request(std::string const &channel, cyng::vector_t &&key, cyng::param_map_t &&data);

        void insert_request(std::string const &channel, cyng::vector_t &&key, cyng::param_map_t &&data);

        void delete_request(std::string const &channel, cyng::vector_t &&key);

        void stop_request(std::string const &channel, cyng::vector_t &&key);

        void query_request(std::string const &channel, cyng::vector_t &&key, ws_sptr);

        void config_request(
            std::string const &channel,
            cyng::vector_t &&key,
            std::string table,
            std::string id,
            std::vector<std::string> &&sections);

      private:
        bus &cluster_bus_;

        cyng::logger logger_;
        std::string const document_root_;

        db &db_;

        std::set<boost::asio::ip::address> const blocklist_;
        std::map<std::string, std::string> const redirects_intrinsic_;
        http::auth_dirs const auths_;

        /**
         * listen for incoming connections.
         */
        http::server server_;

        /**
         * manage config file uploads
         */
        upload upload_;

        /**
         * manage config file downloads
         */
        download download_;

        /**
         * generate unique session tags
         */
        boost::uuids::random_generator uidgen_;

        /**
         * list of all web sockets
         */
        std::map<boost::uuids::uuid, ws_wptr> ws_map_;
    };

    /**
     * Generate the JSON channel update string
     */
    template <typename T> std::string json_update_channel(std::string channel, T value) {
        return cyng::io::to_json(cyng::make_tuple(
            cyng::make_param("cmd", "update"), cyng::make_param("channel", channel), cyng::make_param("value", value)));
    }

    std::string json_insert_record(std::string channel, cyng::tuple_t &&tpl);
    std::string json_update_record(std::string channel, cyng::key_t const &key, cyng::param_t const &param);
    std::string json_update_record(std::string channel, cyng::key_t const &key, cyng::tuple_t &&tpl);
    std::string json_load_icon(std::string channel, bool);
    std::string json_load_level(std::string channel, std::size_t);
    std::string json_delete_record(std::string channel, cyng::key_t const &);
    std::string json_clear_table(std::string channel);

    /**
     * remove all empty records and records that starts with an underline '_' or carry
     * names like "tag" or "source"
     */
    void tidy(cyng::param_map_t &);

} // namespace smf

#endif
