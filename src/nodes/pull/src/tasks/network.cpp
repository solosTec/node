/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#include <tasks/network.h>

#include <tasks/target.h>

#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
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
        std::set<std::string> const& targets)
        : sigs_{
            std::bind(&network::connect, this),     // connect
            std::bind(&network::stop, this, std::placeholders::_1) // stop
        }
        , channel_(wp)
        , ctl_(ctl)
        , tag_(tag)
        , logger_(logger)
        , toggle_(std::move(tgl))
        , targets_(targets)
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
            sp->set_channel_names({"connect", "start"});
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
        //  IP-T targets
        //
        for (auto const &name : targets_) {
            CYNG_LOG_TRACE(logger_, "[ipt] register target " << name);
            auto channel = ctl_.create_named_channel_with_ref<target>(name, ctl_, logger_, bus_);
            channel->dispatch("register", name);

            //
            //  stash this channel
            //
            stash_.lock(channel);

            //
            //  add data output
            //
            // for (auto const &task_name : writer_) {
            //    if (boost::algorithm::starts_with(task_name, "SML:")) {
            //        channel->dispatch("add", task_name);
            //    }
            //}
        }

        //
        //  IEC targets
        //
        // for (auto const &name : iec_targets_) {
        //    CYNG_LOG_TRACE(logger_, "[ipt] register iec target " << name);
        //    auto channel = ctl_.create_named_channel_with_ref<iec_target>(name, ctl_, logger_, bus_);
        //    channel->dispatch("register", name);

        //    //
        //    //  stash this channel
        //    //
        //    stash_.lock(channel);

        //    //
        //    //  add data output
        //    //
        //    for (auto const &task_name : writer_) {
        //        if (boost::algorithm::starts_with(task_name, "IEC:")) {
        //            channel->dispatch("add", task_name);
        //        }
        //    }
        //}

        //
        //  DLMS targets
        //
        // for (auto const &name : dlms_targets_) {
        //    CYNG_LOG_TRACE(logger_, "[ipt] register dlms target " << name);
        //    auto channel = ctl_.create_named_channel_with_ref<dlms_target>(name, ctl_, logger_, bus_);
        //    channel->dispatch("register", name);

        //    //
        //    //  stash this channel
        //    //
        //    stash_.lock(channel);

        //    //
        //    //  add data output
        //    //
        //    for (auto const &task_name : writer_) {
        //        if (boost::algorithm::starts_with(task_name, "DLMS:")) {
        //            channel->dispatch("add", task_name);
        //        }
        //    }
        //}
    }

} // namespace smf
