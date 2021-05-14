/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/sml_target.h>

#include <cyng/io/hex_dump.hpp>
#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>

namespace smf {

    sml_target::sml_target(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, ipt::bus &bus)
        : sigs_{std::bind(&sml_target::register_target, this, std::placeholders::_1), std::bind(&sml_target::receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), std::bind(&sml_target::stop, this, std::placeholders::_1)},
          channel_(wp),
          ctl_(ctl),
          logger_(logger),
          bus_(bus) {
        auto sp = channel_.lock();
        if (sp) {
            sp->set_channel_name("register", 0);
            sp->set_channel_name("receive", 1);
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    sml_target::~sml_target() {
#ifdef _DEBUG_STORE
        std::cout << "sml_target(~)" << std::endl;
#endif
    }

    void sml_target::stop(cyng::eod) {}

    void sml_target::register_target(std::string name) { bus_.register_target(name, channel_); }

    void sml_target::receive(std::uint32_t channel, std::uint32_t source, cyng::buffer_t data, std::string target) {

        if (boost::algorithm::equals(channel_.lock()->get_name(), target)) {
            CYNG_LOG_TRACE(logger_, "[sml target] " << target << " receive " << data.size() << " bytes");
            {
                std::stringstream ss;
                cyng::io::hex_dump<8> hd;
                hd(ss, data.begin(), data.end());
                CYNG_LOG_TRACE(logger_, data.size() << " bytes:\n" << ss.str());
            }

        } else {
            CYNG_LOG_WARNING(
                logger_, "[sml target] wrong target name " << channel_.lock()->get_name() << "/" << target << " - " << data.size()
                                                           << " bytes lost");
        }
    }

} // namespace smf
