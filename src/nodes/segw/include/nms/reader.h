/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_NMS_READER_H
#define SMF_SEGW_NMS_READER_H

#include <cfg.h>
#include <config/cfg_lmn.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/version.h>
#include <cyng/obj/algorithm/reader.hpp>

#include <boost/uuid/string_generator.hpp>

namespace smf {
	namespace nms {

		std::string get_code_name(int status);

		class reader 
		{
			using pmap_reader = cyng::reader<cyng::param_map_t&>;

		public:
			reader(cfg& c, cyng::logger);

			cyng::param_map_t run(cyng::param_map_t&&, std::function<void(boost::asio::ip::tcp::endpoint ep)> rebind);


		private:
			cyng::param_map_t cmd_merge(std::string const& cmd, boost::uuids::uuid, pmap_reader const& dom, std::function<void(boost::asio::ip::tcp::endpoint ep)> rebind);
			cyng::param_map_t cmd_query(std::string const& cmd, boost::uuids::uuid);
			cyng::param_map_t cmd_update(std::string const& cmd, boost::uuids::uuid, pmap_reader const& dom);
			cyng::param_map_t cmd_update_status(std::string const& cmd, boost::uuids::uuid, pmap_reader const& dom);
			cyng::param_map_t cmd_version(std::string const& cmd, boost::uuids::uuid);
			cyng::param_map_t cmd_cm(std::string const& cmd, boost::uuids::uuid, pmap_reader const& dom);

			void cmd_merge_nms(cyng::param_map_t& pm, cyng::param_map_t&& params, std::function<void(boost::asio::ip::tcp::endpoint ep)> rebind);
			void cmd_merge_serial(cyng::param_map_t& pm, cyng::param_map_t&& params);
			void cmd_merge_broker(cyng::param_map_t& pm, lmn_type, cyng::vector_t&&);

		private:
			cyng::logger logger_;
			cfg& cfg_;

			boost::uuids::string_generator sgen_;

			/**
			 * required protocol version
			 */
			static constexpr cyng::version	protocol_version_{ 1, 0 };

		};
	}
}

#endif