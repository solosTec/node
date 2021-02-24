/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_broker.h>
#include <cyng/obj/container_factory.hpp>

namespace smf {

	cfg_broker::cfg_broker(cfg& c, lmn_type type)
		: cfg_(c)
		, type_(type)
	{}

	std::uint8_t cfg_broker::get_index() const {
		return static_cast<std::uint8_t>(type_);
	}
	std::string cfg_broker::get_path_id() const {
		return std::to_string(get_index());
	}

	bool cfg_broker::is_enabled() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "broker-enabled"), false);
	}
	bool cfg_broker::has_login() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "broker-login"), false);
	}

	bool cfg_broker::is_lmn_enabled() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "enabled"), false);
	}

	std::chrono::seconds cfg_broker::get_timeout() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "broker-timeout"), std::chrono::seconds(12));
	}


	std::string cfg_broker::get_port() const {
		return cfg_.get_value(cyng::to_path('/', "lmn", get_path_id(), "port"), "");
	}

	std::string cfg_broker::get_task_name() const {
		return "broker@" + get_port();
	}

	std::size_t cfg_broker::size() const {
		return cfg_.get_value(cyng::to_path('/', "broker", get_path_id(), "count"), static_cast<std::size_t>(0));
	}

	std::string cfg_broker::get_address(std::size_t idx) const {
		return cfg_.get_value(cyng::to_path('/', "broker", get_path_id(), std::to_string(idx), "address"), "");
	}

	std::uint16_t cfg_broker::get_port(std::size_t idx) const {
		return cfg_.get_value(cyng::to_path('/', "broker", get_path_id(), std::to_string(idx), "port"), static_cast<std::uint16_t>(12000));
	}

	std::string cfg_broker::get_account(std::size_t idx) const {
		return cfg_.get_value(cyng::to_path('/', "broker", get_path_id(), std::to_string(idx), "account"), "");
	}

	std::string cfg_broker::get_pwd(std::size_t idx) const {
		return cfg_.get_value(cyng::to_path('/', "broker", get_path_id(), std::to_string(idx), "pwd"), "");
	}

	target cfg_broker::get_target(std::size_t idx) const {
		return target(
			get_account(idx),
			get_pwd(idx),
			get_address(idx),
			get_port(idx)
		);
	}

	target_vec cfg_broker::get_all_targets() const {
		target_vec r;

		for (std::size_t idx = 0; idx < size(); ++idx) {
			r.push_back(get_target(idx));
		}
		return r;
	}

	cyng::vector_t cfg_broker::get_target_vector() const {

		cyng::vector_t vec;
		auto const brokers = get_all_targets();
		std::transform(std::begin(brokers), std::end(brokers), std::back_inserter(vec), [](target const& srv) {
			return cyng::make_object(to_param_map(srv));
		});
		return vec;

	}


	target::target(std::string account
		, std::string pwd
		, std::string address
		, std::uint16_t port)
	: account_(account)
		, pwd_(pwd)
		, address_(address)
		, port_(port)
	{}

	std::string const& target::get_account() const	{
		return account_;
	}

	std::string const& target::get_pwd() const	{
		return pwd_;
	}

	std::string const& target::get_address() const	{
		return address_;
	}

	std::uint16_t target::get_port() const	{
		return port_;
	}

	std::string target::get_login_sequence() const {
		return account_ + ":" + pwd_;
	}


	std::ostream& operator<<(std::ostream& os, target const& brk) {
		os
			<< brk.get_account()
			<< ':'
			<< brk.get_pwd()
			<< '@'
			<< brk.get_address()
			<< ':'
			<< brk.get_port()
			;
		return os;
	}

	cyng::param_map_t to_param_map(target const& srv) {
		return cyng::param_map_factory
			("address", srv.get_address())
			("port", srv.get_port())
			("account", srv.get_account())
			("pwd", srv.get_pwd())
			;
	}


}