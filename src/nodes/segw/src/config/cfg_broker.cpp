/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_broker.h>

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

	target cfg_broker::get_broker(std::size_t idx) const {
		return target(
			get_account(idx),
			get_pwd(idx),
			get_address(idx),
			get_port(idx)
		);
	}

	target_vec cfg_broker::get_broker_list() const {
		target_vec r;

		for (std::size_t idx = 0; idx < size(); ++idx) {
			r.push_back(get_broker(idx));
		}
		return r;
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

}