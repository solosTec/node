/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_BROKER_SEGW_CONFIG_H
#define NODE_BROKER_SEGW_CONFIG_H

#include <string>
#include <chrono>
#include <cstdint>
#include <boost/asio/serial_port_base.hpp>

namespace node
{

	class cache;

	/**
	 * broker
	 */
	class cfg_broker
	{
	public:
		/**
		 * pre-configured data sources
		 */
		enum class source : std::uint8_t {
			WIRELESS_LMN = 1,	//	IF_wMBUS
			WIRED_LMN = 2,		//	rs485
			ETHERNET,
		};

	public:
		class broker {
		public:
			explicit broker(std::string, std::string, std::string, std::uint16_t);
			broker() = delete;
			broker(broker const&) = default;
			broker(broker&&) = default;

			std::string const& get_account() const;
			std::string const& get_pwd() const;
			std::string const& get_address() const;
			std::uint16_t get_port() const;

		private:
			std::string const account_;
			std::string const pwd_;
			std::string const address_;
			std::uint16_t const port_;
		};

		using broker_list_t = std::vector<broker>;
	public:
		cfg_broker(cache&);

		/**
		 * @param tty index to select port (ttyAPP[N], COM[N])
		 * @return number of defined broker nodes
		 */
		 std::vector<broker> get_broker(source s) const;

		 /**
		  * Reference to "broker-login" configuration
		  */
		 bool is_login_required(cfg_broker::source s) const;

		 /**
		  * /dev/ttyAPP0 and /dev/ttyAPP1 on linux systems
		  */
		 std::string get_port_name(cfg_broker::source s) const;

	private:
		broker_list_t get_broker(std::uint8_t) const;

		bool is_login_required(std::string) const;
		std::string get_port_name(std::string) const;

	private:
		cache& cache_;
	};
}
#endif
