/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_SERVER_SEGW_CONFIG_H
#define NODE_SERVER_SEGW_CONFIG_H

#include <cyng/intrinsics/sets.h>

#include <string>
#include <chrono>
#include <cstdint>


namespace node
{
	namespace segw {

		/**
		 * server definition - usefull for broker, listener, ...
		 */
		class server {
		public:
			explicit server(std::string account, std::string pwd, std::string address, std::uint16_t port);
			server() = delete;
			server(server const&) = default;
			server(server&&) noexcept = default;

			std::string const& get_account() const;
			std::string const& get_pwd() const;
			std::string const& get_address() const;
			std::uint16_t get_port() const;

			/**
			 * @return true if the object contains no data
			 */
			bool empty() const;

		private:
			std::string const account_;
			std::string const pwd_;
			std::string const address_;
			std::uint16_t const port_;
		};

	}

	using server_list_t = std::vector<segw::server>;

	/**
	 * convert into a parameter map
	 */
	cyng::param_map_t to_param_map(segw::server const&);

}
#endif
