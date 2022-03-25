/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/dlms_target.h>
#include <tasks/iec_target.h>
#include <tasks/network.h>
#include <tasks/sml_target.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>
#include <cyng/task/controller.h>

#include <boost/algorithm/string/predicate.hpp>
#include <iostream>

namespace smf {

    network::network(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        boost::uuids::uuid tag,
        cyng::logger logger,
        std::string const &node_name,
        std::string const &model,
        ipt::toggle::server_vec_t &&tgl,
        std::vector<std::string> const &config_types,
        std::set<std::string> const &sml_targets,
        std::set<std::string> const &iec_targets,
        std::set<std::string> const &dlms_targets,
        std::vector<std::string> const &writer)
        : sigs_{std::bind(&network::connect, this), std::bind(&network::stop, this, std::placeholders::_1)}
        , channel_(wp)
        , ctl_(ctl)
        , tag_(tag)
        , logger_(logger)
        , toggle_(std::move(tgl))
        , sml_targets_(sml_targets)
        , iec_targets_(iec_targets)
        , dlms_targets_(dlms_targets)
        , writer_(writer)
        , bus_(
              ctl_.get_ctx(),
              logger_,
              std::move(toggle_),
              model,
              std::bind(&network::ipt_cmd, this, std::placeholders::_1, std::placeholders::_2),
              std::bind(&network::ipt_stream, this, std::placeholders::_1),
              std::bind(&network::auth_state, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
        , stash_(ctl_.get_ctx()) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"connect"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void network::stop(cyng::eod) {
        // CYNG_LOG_WARNING(logger_, "stop network task(" << tag_ << ")");
        stash_.stop();
        bus_.stop();
    }

    void network::connect() {
        //
        //	start IP-T bus
        //
        CYNG_LOG_INFO(logger_, "initialize: IP-T client");
        bus_.start();
    }

    void network::ipt_cmd(ipt::header const &h, cyng::buffer_t &&body) {

        CYNG_LOG_TRACE(logger_, "[ipt] cmd " << ipt::command_name(h.command_));
    }
    void network::ipt_stream(cyng::buffer_t &&data) { CYNG_LOG_TRACE(logger_, "[ipt] stream " << data.size() << " byte"); }

    void network::auth_state(bool auth, boost::asio::ip::tcp::endpoint lep, boost::asio::ip::tcp::endpoint rep) {
        if (auth) {
            CYNG_LOG_INFO(logger_, "[ipt] authorized at " << rep);
            BOOST_ASSERT(bus_.is_authorized());
            register_targets();
        } else {
            CYNG_LOG_WARNING(logger_, "[ipt] authorization lost");
            stash_.stop();  //  stop all targets
            stash_.reset(); //  includes clear()
        }
    }

    void network::register_targets() {

        //
        //  SML targets
        //
        for (auto const &name : sml_targets_) {
            CYNG_LOG_TRACE(logger_, "[ipt] register sml target " << name);
            auto channel = ctl_.create_named_channel_with_ref<sml_target>(name, ctl_, logger_, bus_);
            channel->dispatch("register", cyng::make_tuple(name));

            //
            //  stash this channel
            //
            stash_.lock(channel);

            //
            //  add data output
            //
            for (auto const &task_name : writer_) {
                if (boost::algorithm::starts_with(task_name, "SML:")) {
                    channel->dispatch("add", cyng::make_tuple(task_name));
                }
            }
        }

        //
        //  IEC targets
        //
        for (auto const &name : iec_targets_) {
            CYNG_LOG_TRACE(logger_, "[ipt] register iec target " << name);
            auto channel = ctl_.create_named_channel_with_ref<iec_target>(name, ctl_, logger_, bus_);
            channel->dispatch("register", cyng::make_tuple(name));

            //
            //  stash this channel
            //
            stash_.lock(channel);

            //
            //  add data output
            //
            for (auto const &task_name : writer_) {
                if (boost::algorithm::starts_with(task_name, "IEC:")) {
                    channel->dispatch("add", cyng::make_tuple(task_name));
                }
            }
        }

        //
        //  DLMS targets
        //
        for (auto const &name : dlms_targets_) {
            CYNG_LOG_TRACE(logger_, "[ipt] register dlms target " << name);
            auto channel = ctl_.create_named_channel_with_ref<dlms_target>(name, ctl_, logger_, bus_);
            channel->dispatch("register", cyng::make_tuple(name));

            //
            //  stash this channel
            //
            stash_.lock(channel);

            //
            //  add data output
            //
            for (auto const &task_name : writer_) {
                if (boost::algorithm::starts_with(task_name, "DLMS:")) {
                    channel->dispatch("add", cyng::make_tuple(task_name));
                }
            }
        }
    }

} // namespace smf
