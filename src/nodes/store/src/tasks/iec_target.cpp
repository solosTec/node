/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/iec_target.h>

#include <smf/iec/parser.h>
#include <smf/obis/defs.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    iec_target::iec_target(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, ipt::bus &bus)
        : sigs_{std::bind(&iec_target::register_target, this, std::placeholders::_1), std::bind(&iec_target::receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), std::bind(&iec_target::add_writer, this, std::placeholders::_1), std::bind(&iec_target::stop, this, std::placeholders::_1)}
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , bus_(bus)
        , writers_()
        , channel_list_() {
        auto sp = channel_.lock();
        if (sp) {
            std::size_t slot{0};
            sp->set_channel_name("register", slot++);
            sp->set_channel_name("receive", slot++);
            sp->set_channel_name("add", slot++);
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    iec_target::~iec_target() {
#ifdef _DEBUG_STORE
        std::cout << "iec_target(~)" << std::endl;
#endif
    }

    void iec_target::stop(cyng::eod) {}

    void iec_target::register_target(std::string name) { bus_.register_target(name, channel_); }

    void iec_target::receive(std::uint32_t channel, std::uint32_t source, cyng::buffer_t data, std::string target) {

        CYNG_LOG_TRACE(
            logger_,
            "[iec] receive " << data.size() << " bytes from " << channel << ':' << source << '@' << target << " -> "
                             << channel_list_.size() << " consumer(s)");
        BOOST_ASSERT(boost::algorithm::equals(channel_.lock()->get_name(), target));

        //
        //  distribute
        //
        auto const key = ipt::combine(channel, source);
        auto const pos = channel_list_.find(key);
        if (pos != channel_list_.end()) {
            pos->second.parser_.read(data.begin(), data.end());
        } else {
            //
            //  create parser/writer
            //
            auto const r = channel_list_.emplace(
                std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(logger_, writers_));
            if (r.second) {
                r.first->second.parser_.read(data.begin(), data.end());
            }
        }
    }

    void iec_target::add_writer(std::string name) {
        auto channels = ctl_.get_registry().lookup(name);
        if (channels.empty()) {
            CYNG_LOG_WARNING(logger_, "[iec] writer " << name << " not found");
        } else {
            writers_.insert(writers_.end(), channels.begin(), channels.end());
            BOOST_ASSERT(channel_.lock());
            CYNG_LOG_INFO(logger_, "[iec] \"" << channel_.lock()->get_name() << "\" + writer " << name << " #" << writers_.size());
        }
    }

    consumer::consumer(cyng::logger logger, std::vector<cyng::channel_weak> writers)
        : logger_(logger)
        , writers_(writers)
        , parser_(
              [this](cyng::obis code, std::string value, std::string unit) {
                  if (code == OBIS_METER_ADDRESS) {
                      BOOST_ASSERT_MSG(value.size() == 8, "invalid meter id");
                      id_ = value;
                  }
                  CYNG_LOG_TRACE(logger_, "[iec] data \"" << id_ << "\" - " << code << ": " << value << " " << unit);
                  data_.emplace(std::piecewise_construct, std::forward_as_tuple(code), std::forward_as_tuple(value, unit));
              },
              [this](std::string dev, bool crc) {
                  boost::ignore_unused(crc);
                  CYNG_LOG_INFO(logger_, "[iec] readout complete: " << dev << " \"" << id_ << "\"");

                  //
                  //    send to writer(s)
                  //
                  for (auto writer : writers_) {
                      auto sp = writer.lock();
                      if (sp) {
                          sp->dispatch("open", std::string(id_));
                          for (auto const &readout : data_) {
                              sp->dispatch("store", readout.first, readout.second.first, readout.second.second);
                          }
                          sp->dispatch("commit");
                      }
                  }

                  //
                  //    clear data
                  //
                  data_.clear();
                  id_.clear();
              })
        , data_()
        , id_() {}

} // namespace smf
