/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_REDIRECTOR_SEGW_CONFIG_H
#define NODE_LISTENER_SEGW_CONFIG_H

#include <cfg_server.h>
#include "segw.h"

namespace node
{

	class cache;

	/**
	 * broker
	 */
	class cfg_redirector
	{

	public:
		cfg_redirector(cache&);

		/**
		 * It is possible to define one redirector for one serial
		 * hardware port. Currently only redirector for the RS 485 
		 * port is defined
		 * 
		 * @param s index to select port (ttyAPP[N], COM[N])
		 * @return definition of the redirector for the specified
		 * serial hardware port.
		 */
		segw::server get_server(source s) const;

		/**
		 * This is the same as the get_server() function but as a parameter map
		 * 
		 * @return a parameter map for the specified redirector
		 * @see get_server
		 */
		cyng::object get_server_obj(source s) const;


		void set_server(source s, segw::server const&);
		void set_server(std::string const& port_name, segw::server const&);

		bool set_address(source s, std::string const&);
		bool set_address(std::string const& port_name, std::string const&);

		bool set_port(source s, std::uint16_t);
		bool set_port(std::string const& port_name, std::uint16_t);

		bool set_account(source s, std::string const&);
		bool set_account(std::string const& port_name, std::string const&);

		bool set_pwd(source s, std::string const&);
		bool set_pwd(std::string const& port_name, std::string const&);

		/**
		 * Reference to "broker-login" configuration
		 */
		bool is_login_required(source s) const;
		bool set_login_required(source s, bool) const;
		bool set_login_required(std::string const& port_name, bool) const;

		/**
		 * /dev/ttyAPP0 and /dev/ttyAPP1 on linux systems
		 */
		std::string get_port_name(source s) const;

		/**
		 * @return index of specified hardware port [1..2]. If port not exists
		 * returns 0.
		 */
		std::uint8_t get_port_id(std::string const& port_name) const;
		source get_port_source(std::string const& port_name) const;

		/**
		 * Control to start the broker tasks or not.
		 *
		 * @return <PORT>:broker-enabled
		 */
		bool is_enabled(source s) const;
		bool is_enabled(std::string const& port_name) const;
		bool set_enabled(source s, cyng::object obj) const;
		bool set_enabled(std::string const& port_name, cyng::object obj) const;

	private:
		segw::server get_server(std::uint8_t) const;

		bool is_login_required(std::string) const;
		std::string get_port_name(std::string) const;

	private:
		cache& cache_;
	};

}
#endif