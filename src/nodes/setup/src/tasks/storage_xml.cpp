/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/storage_xml.h>

#include <smf/cluster/bus.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

namespace smf {

    storage_xml::storage_xml(cyng::channel_weak wp
		, cyng::controller& ctl
		, bus& cluster_bus
		, cyng::store& cache
		, cyng::logger logger
		, cyng::param_map_t&& cfg)
	: sigs_{
			std::bind(&storage_xml::open, this),
			std::bind(&storage_xml::update, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5),
			std::bind(&storage_xml::insert, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5),
			std::bind(&storage_xml::remove, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			std::bind(&storage_xml::clear, this, std::placeholders::_1, std::placeholders::_2),
			std::bind(&storage_xml::stop, this, std::placeholders::_1),
		}
		, channel_(wp)
		, ctl_(ctl)
		, cluster_bus_(cluster_bus)
		, logger_(logger)
		, store_(cache)
	{
        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"open", "update", "insert", "remove", "clear"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    storage_xml::~storage_xml() {
#ifdef _DEBUG_SETUP
        std::cout << "storage_xml(~)" << std::endl;
#endif
    }

    void storage_xml::open() {}

    void storage_xml::stop(cyng::eod) { CYNG_LOG_WARNING(logger_, "task [" << channel_.lock()->get_name() << "] stopped"); }

    void
    storage_xml::update(std::string table_name, cyng::key_t key, cyng::attr_t attr, std::uint64_t gen, boost::uuids::uuid tag) {}

    void storage_xml::insert(std::string, cyng::key_t key, cyng::data_t data, std::uint64_t gen, boost::uuids::uuid tag) {}

    void storage_xml::remove(std::string, cyng::key_t key, boost::uuids::uuid tag) {}

    void storage_xml::clear(std::string, boost::uuids::uuid tag) {}

} // namespace smf
