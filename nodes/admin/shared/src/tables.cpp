/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "tables.h"
#include <boost/algorithm/string.hpp>

namespace node
{
	/**
	 * Initialize all used table names
	 */
	const tables::array_t	tables::list_ =
	{
		tables::tbl_descr{"TDevice", false, false, false},
		tables::tbl_descr{"TGateway", true, false, false},	//	custom
		tables::tbl_descr{"TLoRaDevice", false, false, false},
		tables::tbl_descr{"TMeter", true, false, false},		//	custom
		tables::tbl_descr{"TMeterAccess", false, false, false},
		tables::tbl_descr{"TLocation", false, false, false},
		tables::tbl_descr{"TGUIUser", false, false, false},
		tables::tbl_descr{"TGWSnapshot", false, false, false},
		tables::tbl_descr{"TNodeNames", false, false, false},
		tables::tbl_descr{"TBridge", true, false, false},	//	custom
		tables::tbl_descr{"_Session", false, false, false},
		tables::tbl_descr{"_Target", false, false, false},
		tables::tbl_descr{"_Connection", false, false, false},
		tables::tbl_descr{"_Cluster", false, false, false},
		tables::tbl_descr{"_Config", false, false, false},
		tables::tbl_descr{"_SysMsg", false, false, false},
		tables::tbl_descr{"_TimeSeries", false, false, false},
		tables::tbl_descr{"_LoRaUplink", false, false, false},
		tables::tbl_descr{"_wMBusUplink", false, false, false},
		tables::tbl_descr{"_IECUplink", false, false, false},
		tables::tbl_descr{"_CSV", false, false, false},
		tables::tbl_descr{"_HTTPSession", false, true, false},	//	local handling
		tables::tbl_descr{"_EventQueue", false, false, true}	//	pass through
	};

	tables::array_t::const_iterator tables::find(std::string name)
	{
		return std::find_if(std::begin(list_), std::end(list_), [&name](tbl_descr const& desc) {
			return boost::algorithm::equals(desc.name_, name);
			});
	}

	bool tables::is_custom(std::string name)
	{
		auto const pos = find(name);

		return (pos != std::end(list_))
			? pos->custom_
			: false
			;
	}

	bool tables::is_local(std::string name)
	{
		auto const pos = find(name);

		return (pos != std::end(list_))
			? pos->local_
			: false
			;
	}

	//
	//	initilize static member
	//
	channel::array_t const channel::rel_{
		channel::rel{"TDevice", "config.device", "table.device.count"},
		channel::rel{"TGateway", "config.gateway", "table.gateway.count"},
		channel::rel{"TMeter", "config.meter", "table.meter.count"},
		channel::rel{"TMeterAccess", "config.meterwMBus", "table.meterwMBus.count"},
		channel::rel{"TLocation", "config.location", "table.location.count"},
		channel::rel{"TBridge", "config.bridge", "table.bridge.count"},
		channel::rel{"TLoRaDevice", "config.lora", "table.LoRa.count"},
		channel::rel{"TGUIUser", "config.user", "table.user.count"},
		channel::rel{"_Session", "status.session", "table.session.count"},
		channel::rel{"_Target", "status.target", "table.target.count"},
		channel::rel{"_Connection", "status.connection", "table.connection.count"},
		channel::rel{"_Config", "config.system", ""},
		channel::rel{"_SysMsg", "monitor.msg", "table.msg.count"},
		channel::rel{"---", "config.web", ""},
		channel::rel{"_HTTPSession", "web.sessions", "table.web.count"},
		channel::rel{"_Cluster", "status.cluster", "table.cluster.count"},
		channel::rel{"_TimeSeries", "monitor.tsdb", ""},
		channel::rel{"_LoRaUplink", "monitor.lora", "table.uplink.count"},
		channel::rel{"_wMBusUplink", "monitor.wMBus", "table.wMBus.count"},
		channel::rel{"_IECUplink", "monitor.IEC", "table.IEC.count"},
		channel::rel{"_CSV", "task.csv", ""},
		channel::rel{"TGWSnapshot", "monitor.snapshot", "table.snapshot.count"},
		channel::rel{"_EventQueue", "event.queue", "table.event.queue"},
	};

	//
	//	channel ----------------------------------------------+
	//
	channel::rel::rel(std::string table, std::string channel, std::string counter)
		: table_(table)
		, channel_(channel)
		, counter_(counter)
	{}

	bool channel::rel::empty() const {
		return table_.empty();
	}

	bool channel::rel::has_counter() const {
		return !counter_.empty();
	}

	channel::rel channel::find_rel_by_table(std::string table)
	{
		auto pos = std::find_if(rel_.begin(), rel_.end(), [table](rel const& rel) {
			return boost::algorithm::equals(table, rel.table_);
			});

		return (pos == rel_.end())
			? rel("", "", "")
			: *pos
			;
	}

	channel::rel channel::find_rel_by_channel(std::string channel)
	{
		auto pos = std::find_if(rel_.begin(), rel_.end(), [channel](rel const& rel) {
			return boost::algorithm::equals(channel, rel.channel_);
			});

		return (pos == rel_.end())
			? rel("", "", "")
			: *pos
			;
	}

	channel::rel channel::find_rel_by_counter(std::string counter)
	{
		auto pos = std::find_if(rel_.begin(), rel_.end(), [counter](rel const& rel) {
			return boost::algorithm::equals(counter, rel.counter_);
			});

		return (pos == rel_.end())
			? rel("", "", "")
			: *pos
			;
	}


}
