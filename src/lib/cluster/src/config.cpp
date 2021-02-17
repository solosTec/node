/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/cluster/config.h>

#include <cyng/obj/algorithm/find.h>
#include <cyng/obj/container_cast.hpp>

#include <iomanip>

namespace smf
{
	server::server()
		: host_()
		, service_()
		, account_()
		, pwd_()
		, salt_(0)
	{}

	server::server(std::string const& host
		, std::string const& service
		, std::string const& account
		, std::string const& pwd
		, std::uint64_t salt)
		: host_(host)
		, service_(service)
		, account_(account)
		, pwd_(pwd)
		, salt_(salt)
	{}

	std::ostream& operator<<(std::ostream& os, server const& srv) {
		os
			<< srv.account_
			<< ':'
			<< srv.pwd_
			<< '@'
			<< srv.host_
			<< ':'
			<< srv.service_
			;
		return os;
	}

	server read_config(cyng::param_map_t const& pmap) {

		return server(
			cyng::find_value(pmap, std::string("host"), std::string()),
			cyng::find_value(pmap, std::string("service"), std::string()),
			cyng::find_value(pmap, std::string("account"), std::string()),
			cyng::find_value(pmap, std::string("pwd"), std::string()),
			cyng::find_value(pmap, std::string("salt"), 12));
	}

	toggle::server_vec_t read_config(cyng::vector_t const& vec) {
		toggle::server_vec_t srv_vec;

		std::transform(std::begin(vec), std::end(vec), std::back_inserter(srv_vec), [](cyng::object const& obj) {
			auto const pmap = cyng::container_cast<cyng::param_map_t>(obj);
			return read_config(pmap);
			});
		return srv_vec;
	}

	toggle::toggle(server_vec_t&& cfg)
		: cfg_(std::move(cfg))
		, active_(0)
	{}

	server toggle::changeover() const {
		if (!cfg_.empty())	active_++;
		if (active_ == cfg_.size())	active_ = 0;
		return get();
	}

	/**
	 * get current reduncancy
	 */
	server toggle::get() const {
		return (cfg_.empty())
			? server()
			: cfg_.at(active_)
			;
	}

}



