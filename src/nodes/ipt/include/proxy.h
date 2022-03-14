/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_PROXY_H
#define SMF_IPT_PROXY_H

#include <smf/cluster/bus.h>
#include <smf/obis/tree.hpp>
#include <smf/sml/generator.h>
#include <smf/sml/reader.h>
#include <smf/sml/unpack.h>

#include <cyng/log/logger.h>
#include <cyng/store/db.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>
#include <queue>
#include <variant>

namespace smf {

    // template <class... Ts> struct overload : Ts... { using Ts::operator()...; };
    // template <class... Ts> overload(Ts...) -> overload<Ts...>; // line not needed in C++20...

    class ipt_session;

    /**
     * SML proxy
     */
    class proxy {

        struct evt_init {};
        struct evt_get_proc_parameter_response {
            cyng::buffer_t srv_;
            cyng::obis_path_t path_;
            cyng::obis code_;
            cyng::attr_t attr_;
            cyng::tuple_t tpl_;
        };
        struct evt_get_profile_list_response {
            cyng::buffer_t srv_;
            cyng::obis_path_t path_;
            cyng::object act_time_;
            std::chrono::seconds reg_time_;
            cyng::object val_time_;
            std::uint32_t mbus_state_;
            sml::sml_list_t list_;
        };
        struct evt_close_response {
            std::string trx_;
            cyng::tuple_t msg_;
        };

        struct initial_s {
            inline void on(proxy &, evt_init) {}
            inline void on(proxy &, evt_get_proc_parameter_response &&) {}
            inline void on(proxy &, evt_close_response &&) {}
            inline void on(proxy &, evt_get_profile_list_response &&) {}
            inline bool get_online_state() const { return false; }
        };
        struct backup_s {
            enum class query_state { OFF, QRY_BASIC, QRY_METER, QRY_ACCESS, QRY_USER } state_;
            boost::uuids::uuid tag_; // database key "gateway"
            cyng::buffer_t id_;      // gateway id
            std::string fw_;         // firware version
            std::chrono::system_clock::time_point start_time_;
            std::queue<cyng::buffer_t> meters_;    //!< temporary data
            std::queue<cyng::obis_path_t> access_; //!< temporary data
            std::map<cyng::buffer_t, sml::tree_t> cfg_;
            std::string get_state() const;
            void on(proxy &, evt_init);
            void on(proxy &, evt_get_proc_parameter_response &&);
            void on(proxy &, evt_close_response &&);
            inline void on(proxy &, evt_get_profile_list_response &&) {}
            bool get_online_state() const;

            void cfg_backup_meter(proxy &prx);
            void cfg_backup_access(proxy &prx);
            void cfg_backup_user(proxy &prx);

            void complete(proxy &prx);
            void update_connect_state(ipt_session &ipts, bool);
            /**
             * send to configuration manager
             */
            std::size_t backup(proxy &prx, boost::uuids::uuid tag, cyng::buffer_t gw, cyng::tuple_t cl);
        };
        struct get_proc_param_req_s {
            boost::uuids::uuid tag_;
            cyng::buffer_t id_; // gateway id
            cyng::obis root_;
            boost::uuids::uuid source_;
            boost::uuids::uuid rpeer_;
            bool online_;
            void on(proxy &, evt_init);
            void on(proxy &, evt_get_proc_parameter_response &&);
            void on(proxy &, evt_close_response &&);
            inline void on(proxy &, evt_get_profile_list_response &&) {}
            inline bool get_online_state() const { return online_; }
        };
        struct set_proc_param_req_s {
            boost::uuids::uuid tag_; // "gateway" table key
            cyng::buffer_t id_;      // gateway id
            cyng::obis root_;
            cyng::param_map_t params_;
            boost::uuids::uuid source_;
            boost::uuids::uuid rpeer_;
            bool online_;
            void on(proxy &, evt_init);
            inline void on(proxy &, evt_get_proc_parameter_response &&) {}
            void on(proxy &, evt_close_response &&);
            inline void on(proxy &, evt_get_profile_list_response &&) {}
            inline bool get_online_state() const { return online_; }
        };
        struct get_profile_list_req_s {
            boost::uuids::uuid tag_;
            cyng::buffer_t id_; // gateway id
            cyng::obis root_;
            cyng::param_map_t params_;
            boost::uuids::uuid source_;
            boost::uuids::uuid rpeer_;
            bool online_;
            void on(proxy &, evt_init);
            inline void on(proxy &, evt_get_proc_parameter_response &&) {}
            void on(proxy &, evt_close_response &&);
            void on(proxy &, evt_get_profile_list_response &&);
            inline bool get_online_state() const { return online_; }
        };
        using state = std::variant<initial_s, backup_s, get_proc_param_req_s, set_proc_param_req_s, get_profile_list_req_s>;

      public:
        proxy(cyng::logger, ipt_session &, bus &cluster_bus, cyng::mac48 const &);

        void cfg_backup(
            std::string name,
            std::string pwd,
            boost::uuids::uuid key,
            cyng::buffer_t id,
            std::string fw, //  firware version
            std::chrono::system_clock::time_point tp);

        void get_proc_param_req(
            std::string name,
            std::string pwd,
            boost::uuids::uuid tag,
            cyng::buffer_t id,
            cyng::obis section,
            boost::uuids::uuid source,
            cyng::obis_path_t,
            boost::uuids::uuid tag_cluster);

        void set_proc_param_req(
            std::string name,
            std::string pwd,
            boost::uuids::uuid tag,
            cyng::buffer_t id,
            cyng::obis section,
            cyng::param_map_t params,
            boost::uuids::uuid source,
            cyng::obis_path_t,
            boost::uuids::uuid tag_cluster);

        void get_profile_list_req(
            std::string name,
            std::string pwd,
            boost::uuids::uuid tag,
            cyng::buffer_t id,
            cyng::obis section,
            cyng::param_map_t params,
            boost::uuids::uuid source,
            boost::uuids::uuid tag_cluster);

        void read(cyng::buffer_t &&data);

        bool is_online() const;

      private:
        template <typename Event> void on(Event &&evt) {
            //  call state specific function
            std::visit([&, this](auto &s) { s.on(*this, std::forward<Event>(evt)); }, state_);
            // std::visit(
            //     overload{
            //         [&, this](initial_s &s) { s.on(*this, std::forward<Event>(evt)); },
            //         [&, this](backup_s &s) { s.on(*this, std::forward<Event>(evt)); },
            //         [&, this](get_profile_list_req_s &s) { s.on(*this, std::forward<Event>(evt)); }},
            //     state_);
        }

      private:
        cyng::logger logger_;
        ipt_session &session_;
        bus &cluster_bus_;
        cyng::mac48 const client_id_;

        state state_;

        sml::unpack parser_;
        sml::request_generator req_gen_;
        std::size_t throughput_;
    };

} // namespace smf

#endif
