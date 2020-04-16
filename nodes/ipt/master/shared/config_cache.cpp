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

namespace node 
{
	config_cache::config_cache(cyng::buffer_t srv, sml::obis_path&& sections)
        : srv_(srv)
        , sections_(init_sections(std::move(sections)))
	{ }

    bool config_cache::update(sml::obis root, cyng::param_map_t const& params)
    {
        //
        //  selective update
        //
        auto pos = sections_.find(sml::obis_path{ root });
        if (pos != sections_.end()) {
            pos->second = params;
            return true;
        }
        return false;
    }

    bool config_cache::update(sml::obis_path path, cyng::param_map_t const& params, bool force)
    {
        //
        //  selective update
        //
        auto pos = sections_.find(path);
        if (pos != sections_.end()) {
            pos->second = params;
            return true;
        }
        if (force) {
            sections_.emplace(path, params);
            return true;
        }
        return false;
    }

    //  static
    config_cache::sections_t config_cache::init_sections(sml::obis_path&& slots)
    {
        sections_t secs;
        for (auto const& code : slots) {
            secs.emplace(sml::obis_path{ code }, cyng::param_map_t());
        }
        return secs;
    }

    std::string config_cache::get_server() const
    {
        return sml::from_server_id(srv_);
    }

    bool config_cache::is_cached(sml::obis code) const
    {
        return is_cached(sml::obis_path{ code });
    }

    bool config_cache::is_cached(sml::obis_path const& path) const
    {
        return sections_.find(path) != sections_.end();
    }


    void config_cache::add(sml::obis_path&& path)
    {
        if (!is_cached(path)) {
            sections_.emplace(path, cyng::param_map_t());
        }
    }

    void config_cache::remove(sml::obis_path&& slots)
    {
        for (auto const& code : slots) {
            if (is_cached(code)) {
                sections_.erase(sml::obis_path{ code });
            }
        }
    }

    void config_cache::clear()
    {
        sections_.clear();
    }

    cyng::param_map_t config_cache::get_section(sml::obis_path const& path) const
    {
        auto const pos = sections_.find(path);
        return (pos != sections_.end())
            ? pos->second 
            : cyng::param_map_t{}
        ;
    }

    cyng::param_map_t config_cache::get_section(sml::obis root) const
    {
        return get_section(sml::obis_path({ root }));
    }


    void config_cache::loop_access_rights(cb_access_rights cb) const
    {
        auto const section = get_section(sml::OBIS_ROOT_ACCESS_RIGHTS);

        //
        //  loop over all "roles"
        //
        for (auto const& rec_role : section) {

            //  convert name to OBIS code
            auto const r = sml::parse_obis(rec_role.first);
            if (r.second) {

                //
                //  get role
                //
                std::uint8_t const role = r.first.get_quantities();
                auto const pm = cyng::to_param_map(rec_role.second);
                if (!pm.empty()) {

                    //
                    //  loop over all "user" of this "role"
                    //
                    for (auto const& rec_user : pm) {

                        //  convert name to OBIS code
                        auto const r = sml::parse_obis(rec_user.first);
                        if (r.second) {

                            BOOST_ASSERT(role == r.first.get_quantities());

                            //
                            //  get user
                            //
                            std::uint8_t const user = r.first.get_storage();
                            auto params = cyng::to_param_map(rec_user.second);

                            //
                            //  get devices/server
                            //  walk over the parameter list an select all devices
                            //
                            auto const devices = split_list_of_access_rights(params);

                            //
                            // get value 81818161FFFF
                            //
                            auto const name = get_user_name(params);

                            //
                            //  callback
                            //
                            cb(role, user, name, devices);
                        }
                    }
                }
            }
        }
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

}
