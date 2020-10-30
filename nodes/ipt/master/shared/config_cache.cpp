/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#include "config_cache.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/parser/obis_parser.h>
#include <smf/sml/srv_id_io.h>

#include <cyng/object.h>
#include <cyng/set_cast.h>
#include <cyng/buffer_cast.h>
#include <cyng/factory.h>

namespace node 
{
	config_cache::config_cache(cyng::buffer_t srv, sml::obis_path_t&& sections)
        : srv_(srv)
        , sections_(init_sections(std::move(sections)))
	{ }

    void config_cache::reset(cyng::buffer_t srv, sml::obis_path_t&& sections)
    {
        srv_ = srv;
        sections_.clear();
        sections_ = init_sections(std::move(sections));
    }

    bool config_cache::update(sml::obis root, cyng::param_t const& param)
    {
        auto const path = sml::obis_path_t{ root };
        return update(path, param);
    }

    bool config_cache::update(sml::obis_path_t path, cyng::param_t const& param)
    {
        if (is_cache_allowed(path)) {
            //
            //  selective update
            //
            auto pos = sections_.find(path);
            if (pos != sections_.end()) {
                pos->second = param.second;
                return true;
            }
            else {
                sections_.emplace(path, param.second);
            }
        }
        return false;
    }

    //  static
    config_cache::sections_t config_cache::init_sections(sml::obis_path_t&& slots)
    {
        sections_t secs;
        for (auto const& code : slots) {
            if (is_cache_allowed(code)) {
                secs.emplace(sml::obis_path_t{ code }, cyng::make_object());
            }
        }
        return secs;
    }

    std::string config_cache::get_server() const
    {
        return sml::from_server_id(srv_);
    }

    bool config_cache::is_cached(sml::obis code) const
    {
        return is_cached(sml::obis_path_t{ code });
    }

    bool config_cache::is_cached(sml::obis_path_t const& path) const
    {
        return sections_.find(path) != sections_.end();
    }

    void config_cache::add(cyng::buffer_t srv, sml::obis_path_t const& path)
    {
        if (!is_cached(path)) {
            srv_ = srv;
            if (is_cache_allowed(path)) {
                sections_.emplace(path, cyng::make_object());
            }
        }
    }

    void config_cache::remove(sml::obis_path_t&& slots)
    {
        for (auto const& code : slots) {
            if (is_cached(code)) {
                sections_.erase(sml::obis_path_t{ code });
            }
        }
    }

    cyng::object config_cache::get_section(sml::obis_path_t const& path) const
    {
        auto const pos = sections_.find(path);
        return (pos != sections_.end())
            ? pos->second 
            : cyng::make_object()
        ;
    }

    cyng::object config_cache::get_section(sml::obis root) const
    {
        return get_section(sml::obis_path_t({ root }));
    }


    void config_cache::loop_access_rights(cb_access_rights cb) const
    {
        auto const section = get_section(sml::OBIS_ROOT_ACCESS_RIGHTS);
        BOOST_ASSERT_MSG(false, "ToDo");

        //
        //  loop over all "roles"
        //
        //for (auto const& rec_role : section) {

        //    //  convert name to OBIS code
        //    auto const r = sml::parse_obis(rec_role.first);
        //    if (r.second) {

        //        //
        //        //  get role
        //        //
        //        std::uint8_t const role = r.first.get_quantities();
        //        auto const pm = cyng::to_param_map(rec_role.second);

        //        //
        //        //  loop over all "user" of this "role"
        //        //
        //        for (auto const& rec_user : pm) {

        //            //  convert name to OBIS code
        //            auto const r = sml::parse_obis(rec_user.first);
        //            if (r.second) {

        //                BOOST_ASSERT(role == r.first.get_quantities());

        //                //
        //                //  get user
        //                //
        //                std::uint8_t const user = r.first.get_storage();
        //                auto params = cyng::to_param_map(rec_user.second);

        //                //
        //                //  get devices/server
        //                //  walk over the parameter list an select all devices
        //                //
        //                auto const devices = split_list_of_access_rights(params);

        //                //
        //                // get value 81818161FFFF
        //                //
        //                auto const name = get_user_name(params);

        //                //
        //                //  callback
        //                //
        //                cb(role, user, name, devices);
        //            }
        //        }
        //    }
        //}
    }

    sml::obis_path_t config_cache::get_root_sections() const
    {
        sml::obis_path_t path;
        for (auto const& value : sections_) {
            if (value.first.size() == 1) {
                path.push_back(value.first.front());
            }
        }
        return path;
    }

    config_cache::device_map_t split_list_of_access_rights(cyng::param_map_t& params)
    {
        config_cache::device_map_t devices;

        for (auto pos = params.begin(); pos != params.end(); ) {
            auto const r = sml::parse_obis(pos->first);
            if (r.second) {
                //  81 81 81 64 01 SS (SS == server identifier)
                if (r.first.is_matching(0x81, 0x81, 0x81, 0x64, 0x01).second) {

                    auto const srv = cyng::to_buffer(pos->second);
                    if (!srv.empty()) {

                        //
                        //  get devices type
                        //
                        if (sml::get_srv_type(srv) < 4) {

                            //
                            //  ignore other server types, especially actors
                            //
                            //devices.insert(*pos);
                            devices.emplace(r.first, srv);
                        }
                    }

                    //
                    //  remove parameter
                    //
                    pos = params.erase(pos);
                }
                else {
                    //  next parameter
                    ++pos;
                }
            }
            else {
                //  next parameter
                ++pos;
            }
        }

        return devices;
    }

    std::string get_user_name(cyng::param_map_t const& params)
    {
        auto const pos = params.find(sml::OBIS_ACCESS_USER_NAME.to_str());
        return (pos != params.end())
            ? cyng::value_cast<std::string>(pos->second, "")
            : ""
            ;
    }

    bool is_cache_allowed(sml::obis code)
    {
        switch (code.to_uint64()) {
        case sml::CODE_ROOT_SECURITY:
        case sml::CODE_ROOT_PUSH_OPERATIONS:
        case sml::CODE_ROOT_GPRS_PARAM:
        case sml::CODE_ROOT_IPT_PARAM:
        case sml::CODE_ROOT_INVISIBLE_DEVICES:
        case sml::CODE_ROOT_VISIBLE_DEVICES:
        case sml::CODE_ROOT_NEW_DEVICES:
        case sml::CODE_ROOT_ACTIVE_DEVICES:
        case sml::CODE_ROOT_DEVICE_INFO:
        case sml::CODE_ROOT_ACCESS_RIGHTS:
        case sml::CODE_ROOT_FIRMWARE:
        case sml::CODE_ROOT_DATA_COLLECTOR:
        case sml::CODE_ROOT_NTP:
        case sml::CODE_ROOT_DEVICE_IDENT:
        case sml::CODE_ROOT_W_MBUS_STATUS:
        case sml::CODE_IF_wMBUS:
        case sml::CODE_ROOT_BROKER:
        case sml::CODE_ROOT_SERIAL:
            return true;
        default:
            break;
        }
        return false;
    }

    bool is_cache_allowed(sml::obis_path_t const& path)
    {
        //
        //  only the root element is relevant
        //
        return is_cache_allowed(path.front());
    }


}
