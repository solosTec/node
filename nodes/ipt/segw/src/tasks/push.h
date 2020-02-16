/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_PUSH_H
#define NODE_SEGW_TASK_PUSH_H

#include <smf/sml/intrinsics/obis.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/table/table_fwd.h>

#include <boost/predef.h>	//	requires Boost 1.55

namespace node
{
	/**
	 * If data available send it to tsk
	 */
	class cache;
	class storage;
	namespace ipt {
		class bus;
	}
	namespace sml {
		class res_generator;
		class trx;
	}
	class push
	{
	public:
		//	[0] write entry
		using msg_0 = std::tuple<bool, std::uint32_t, std::uint32_t, std::uint16_t, std::uint32_t>;
		using msg_1 = std::tuple<std::string>;	//	target
		using msg_2 = std::tuple<std::string, std::uint32_t>;	//	interval, delay


		using signatures_t = std::tuple<msg_0, msg_1, msg_2>;

	public:
		push(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cache&
			, storage&
			, cyng::buffer_t srv_id
			, std::uint8_t nr
			, cyng::buffer_t profile
			, std::chrono::seconds interval
			, std::chrono::seconds delay
			, std::string target
			, ipt::bus*
			, std::size_t tsk);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - write an entry
		 *
		 */
		cyng::continuation process(bool success
			, std::uint32_t channel
			, std::uint32_t source
			, std::uint16_t status	//	u8!
			, std::uint32_t count);

		/**
		 * @brief slot [1] - target name changed
		 *
		 */
		cyng::continuation process(std::string);

		/**
		 * @brief slot [2] - interval/delay changed
		 *
		 */
		cyng::continuation process(std::string, std::uint32_t);

	private:
		bool send_push_data(sml::res_generator&
			, sml::trx&
			, std::uint32_t channel
			, std::uint32_t source);
		std::vector<sml::obis> collect_obis_codes(cyng::store::table const* tbl_dm);

		void collect_profile_8181C78610FF(sml::res_generator&, sml::trx&, std::uint32_t channel, std::uint32_t source);
		void collect_profile_8181C78611FF(sml::res_generator&, sml::trx&, std::uint32_t channel, std::uint32_t source);
		void collect_profile_8181C78612FF(sml::res_generator&, sml::trx&, std::uint32_t channel, std::uint32_t source);
		void collect_profile_8181C78613FF(sml::res_generator&, sml::trx&, std::uint32_t channel, std::uint32_t source);
		void collect_profile_8181C78614FF(sml::res_generator&, sml::trx&, std::uint32_t channel, std::uint32_t source);
		void collect_profile_8181C78615FF(sml::res_generator&, sml::trx&, std::uint32_t channel, std::uint32_t source);
		void collect_profile_8181C78616FF(sml::res_generator&, sml::trx&, std::uint32_t channel, std::uint32_t source);
		void collect_profile_8181C78617FF(sml::res_generator&, sml::trx&, std::uint32_t channel, std::uint32_t source);
		void collect_profile_8181C78618FF(sml::res_generator&, sml::trx&, std::uint32_t channel, std::uint32_t source);

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * permanent storage
		 */
		cache& cache_;

		/**
		 * permanent storage
		 */
		storage& storage_;

		/** 
		 * srv_id_ and nr_ are the key for table "_DataCollector"
		 */
		cyng::buffer_t const srv_id_;
		std::uint8_t const nr_;

		/**
		 * as stored in table "_DataCollector"
		 */
		sml::obis const profile_;	

		std::chrono::seconds interval_;
		std::chrono::seconds delay_;
		std::string target_;

		ipt::bus* ipt_bus_;
	};

	std::chrono::system_clock::time_point get_ts(sml::obis profile, std::uint64_t tsidx);

}
#if BOOST_COMP_GNUC
namespace cyng {
	namespace async {

		//
		//	initialize static slot names
		//
		template <>
		std::map<std::string, std::size_t> cyng::async::task<node::push>::slot_names_;
	}
}
#endif
namespace cyng {
	namespace async {

		//
		//	initialize static slot names
		//
		template <>
		std::map<std::string, std::size_t> task<node::push>::slot_names_({
			{ "target", 1 },
			{ "interval", 2 },
			{ "delay", 2 },
			{ "service", 3 }
			});

	}
}

#endif