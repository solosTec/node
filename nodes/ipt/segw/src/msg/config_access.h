/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_CONFIG_ACCESS_H
#define NODE_SEGW_CONFIG_ACCESS_H


#include <smf/sml/intrinsics/obis.h>

#include <cyng/log.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/store/store_fwd.h>

namespace node
{
	//
	//	forward declaration
	//
	class cache;
	namespace sml
	{

		/**
		 * manage security parameters
		 *
		 * 99 00 00 00 00 05: FTP Update
		 * 81 81 27 32 07 01: operation log book RP
		 * 00 00 60 08 00 FF: operating seconds counter
		 * 81 00 60 05 00 00:
		 * 01 00 00 09 0B 00:
		 * 81 00 00 09 0B 01:
		 * 81 04 00 06 00 FF:
		 * 81 04 00 07 00 FF:
		 * 81 04 02 07 00 FF:
		 * 81 49 0D 06 00 FF:
		 * 81 49 0D 07 00 FF:
		 * 81 04 0D 07 00 FF:
		 * 81 04 0D 06 00 FF:
		 * 81 04 0D 08 00 FF:
		 * 81 04 18 07 00 FF:
		 * 81 04 0E 06 00 FF:
		 * 81 06 0F 06 00 FF:
		 * 81 06 19 07 00 FF:
		 * 81 81 C7 8A 01 FF:
		 * 81 81 11 06 FF FF:
		 * 81 81 10 06 FF FF:
		 * 81 81 11 06 FB FF:
		 * 81 81 11 06 FC FF:
		 * 81 81 11 06 FD FF:
		 * 81 81 C7 83 82 01:
		 * 81 81 C7 82 01 FF:
		 * 81 81 00 02 00 01:
		 * 81 81 C7 83 82 07:
		 * 81 81 C7 88 10 FF:
		 * 81 81 C7 88 01 FF:
		 * 81 81 81 60 FF FF:
		 * 81 02 00 07 00 FF: ROOT_CUSTOM_INTERFACE (end customer interface parameter)
		 * 81 02 00 07 10 FF: ROOT_CUSTOM_PARAM (end customer interface status)
		 * 81 81 C7 89 E1 FF:
		 * 81 81 C7 90 00 FF:
		 * 81 81 C7 91 00 FF:
		 * 81 81 C7 92 00 FF:
		 */
		class res_generator;
		class config_access
		{
		public:
			config_access(cyng::logging::log_ptr
				, res_generator& sml_gen
				, cache& c);

			void get_proc_params(std::string trx, cyng::buffer_t srv_id, obis_path_t) const;
			void set_param(node::sml::obis code
				, cyng::buffer_t srv_id
				, cyng::param_t const& param);

		private:
			void collect_role(cyng::tuple_t& msg
				, role
				, cyng::store::table const* tblUser
				, cyng::store::table const* tblPriv) const;

			void collect_privs(cyng::tuple_t& msg
				, obis code
				, std::uint8_t user
				, cyng::buffer_t id
				, cyng::store::table const* tblPriv) const;

		private:
			cyng::logging::log_ptr logger_;


			/**
			 * buffer for current SML message
			 */
			res_generator& sml_gen_;

			/**
			 * configuration db
			 */
			cache& cache_;

		};
	}	//	sml
}

#endif
