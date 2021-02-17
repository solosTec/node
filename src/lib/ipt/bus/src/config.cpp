/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/ipt/config.h>
#include <smf/ipt/scramble_key_format.h>

#include <cyng/obj/algorithm/find.h>

namespace smf
{
	namespace ipt
	{
		
		server::server()
		: host_()
			, service_()
			, account_()
			, pwd_()
			, sk_()
			, scrambled_(true)
			, monitor_(0)
		{}

		server::server(std::string const& host
			, std::string const& service
			, std::string const& account
			, std::string const& pwd
			, scramble_key const& sk
			, bool scrambled
			, int monitor)
		: host_(host)
			, service_(service)
			, account_(account)
			, pwd_(pwd)
			, sk_(sk)
			, scrambled_(scrambled)
			, monitor_(monitor)
		{}

		server read_config(cyng::param_map_t const& pmap) {

			return server(
				cyng::find_value(pmap, std::string("host"), std::string()),
				cyng::find_value(pmap, std::string("service"), std::string()),
				cyng::find_value(pmap, std::string("account"), std::string()),
				cyng::find_value(pmap, std::string("pwd"), std::string()),
				to_sk(cyng::find_value(pmap, std::string("def-sk"), std::string("0102030405060708090001020304050607080900010203040506070809000001"))),
				cyng::find_value(pmap, std::string("scrambled"), true),
				cyng::find_value(pmap, std::string("monitor"), 12));
		}

	}	//	ipt
}



