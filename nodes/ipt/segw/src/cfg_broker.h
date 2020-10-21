/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_BROKER_SEGW_CONFIG_H
#define NODE_BROKER_SEGW_CONFIG_H

#include <cyng/intrinsics/sets.h>

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
			OTHER = 0,
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
			broker(broker&&) noexcept = default;

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
		  * @return a vector aof all brokers
		  */
		 cyng::vector_t get_broker_vector(source s) const;

		 void set_broker(source s, std::uint8_t idx, broker const&);
		 void set_broker(std::string const& port_name, std::uint8_t idx, broker const&);

		 bool set_address(source s, std::uint8_t idx, std::string const&);
		 bool set_address(std::string const& port_name, std::uint8_t idx, std::string const&);

		 bool set_port(source s, std::uint8_t idx, std::uint16_t);
		 bool set_port(std::string const& port_name, std::uint8_t idx, std::uint16_t);

		 bool set_account(source s, std::uint8_t idx, std::string const&);
		 bool set_account(std::string const& port_name, std::uint8_t idx, std::string const&);

		 bool set_pwd(source s, std::uint8_t idx, std::string const&);
		 bool set_pwd(std::string const& port_name, std::uint8_t idx, std::string const&);

		 /**
		  * Reference to "broker-login" configuration
		  */
		 bool is_login_required(cfg_broker::source s) const;
		 bool set_login_required(cfg_broker::source s, bool) const;
		 bool set_login_required(std::string const& port_name, bool) const;

		 /**
		  * /dev/ttyAPP0 and /dev/ttyAPP1 on linux systems
		  */
		 std::string get_port_name(cfg_broker::source s) const;

		 /**
		  * @return index of specified hardware port [1..2]. If port not exists
		  * returns 0.
		  */
		 std::uint8_t get_port_id(std::string const& port_name) const;
		 cfg_broker::source get_port_source(std::string const& port_name) const;

		 /**
		  * Control to start the broker tasks or not.
		  *
		  * @return <PORT>:broker-enabled
		  */
		 bool is_enabled(cfg_broker::source s) const;
		 bool is_enabled(std::string const& port_name) const;
		 bool set_enabled(cfg_broker::source s, cyng::object obj) const;
		 bool set_enabled(std::string const& port_name, cyng::object obj) const;

	private:
		broker_list_t get_broker(std::uint8_t) const;

		bool is_login_required(std::string) const;
		std::string get_port_name(std::string) const;

	private:
		cache& cache_;
	};

	/**
	 * convert into a parameter map
	 */
	cyng::param_map_t to_param_map(cfg_broker::broker const&);

}
#endif
