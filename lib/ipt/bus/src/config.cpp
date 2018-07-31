/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include <smf/ipt/config.h>
#include <smf/ipt/scramble_key_format.h>
#include <cyng/dom/reader.h>

namespace node
{
	namespace ipt
	{
		master_record load_cluster_rec(cyng::tuple_t const& cfg)
		{
			auto dom = cyng::make_reader(cfg);
			return master_record(cyng::value_cast<std::string>(dom.get("host"), "")
				, cyng::value_cast<std::string>(dom.get("service"), "")
				, cyng::value_cast<std::string>(dom.get("account"), "")
				, cyng::value_cast<std::string>(dom.get("pwd"), "")
				, from_string(cyng::value_cast<std::string>(dom.get("def-sk"), "0102030405060708090001020304050607080900010203040506070809000001"))
				, cyng::value_cast(dom.get("scrambled"), true)
				, cyng::value_cast(dom.get("monitor"), 12));
		}

		master_config_t load_cluster_cfg(cyng::vector_t const& cfg)
		{
			master_config_t cluster_cfg;
			cluster_cfg.reserve(cfg.size());

			std::for_each(cfg.begin(), cfg.end(), [&cluster_cfg](cyng::object const& obj) {
				cyng::tuple_t tmp;
				cluster_cfg.push_back(load_cluster_rec(cyng::value_cast(obj, tmp)));
			});

			return cluster_cfg;
		}

		master_record::master_record()
			: host_()
			, service_()
			, account_()
			, pwd_()
			, sk_()
			, scrambled_(true)
			, monitor_(0)
		{}

		master_record::master_record(std::string const& host
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

		redundancy::redundancy(master_config_t const& cfg)
			: config_(cfg)
			, master_(0)
		{
			BOOST_ASSERT(!config_.empty());
		}

		bool redundancy::next() const
		{
			if (!config_.empty())
			{
				master_++;
				if (master_ == config_.size())
				{
					master_ = 0;
				}
				return true;
			}
			return false;
		}

		master_record const& redundancy::get() const
		{
			return config_.at(master_);
		}

	}
}
