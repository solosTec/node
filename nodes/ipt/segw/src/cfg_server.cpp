/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <cfg_server.h>

 #include <cyng/factory/set_factory.h>


namespace node
{
	namespace segw {

		server::server(std::string account, std::string pwd, std::string address, std::uint16_t port, bool enabled)
			: account_(account)
			, pwd_(pwd)
			, address_(address)
			, port_(port)
			, enabled_(enabled)
		{}

		std::string const& server::get_account() const
		{
			return account_;
		}

		std::string const& server::get_pwd() const
		{
			return pwd_;
		}

		std::string const& server::get_address() const
		{
			return address_;
		}

		std::uint16_t server::get_port() const
		{
			return port_;
		}

		bool server::is_enabled() const
		{
			return enabled_;
		}

		bool server::empty() const
		{
			return (port_ == 0)
				&& account_.empty()
				;
		}

	}

	cyng::param_map_t to_param_map(segw::server const& target)
	{
		return cyng::param_map_factory
			("address", target.get_address())
			("port", target.get_port())
			("account", target.get_account())
			("pwd", target.get_pwd())
			("enabled", target.is_enabled())
			;
	}

}
