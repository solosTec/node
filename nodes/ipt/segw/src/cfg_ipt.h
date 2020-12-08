/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_SEGW_CONFIG_H
#define NODE_IPT_SEGW_CONFIG_H

#include <smf/sml/status.h>
#include <smf/ipt/config.h>

#include <cyng/store/db.h>
#include <cyng/io/serializer.h>

#include <boost/uuid/uuid.hpp>
#include <boost/algorithm/string.hpp>

namespace node
{

	class cache;
	class cfg_ipt
	{
	public:
		cfg_ipt(cache&);

		/** @brief 81 48 27 32 06 01
		 *
		 * @return wait before TCP/IP reconnect (seconds)
		 */
		std::chrono::seconds get_ipt_tcp_wait_to_reconnect();

		/** @brief 81 48 31 32 02 01
		 *
		 * A value of 0 implies that no attempts are made to initiate a connection.
		 *
		 * @return max. number of retries to reconnect.
		 */
		std::uint32_t get_ipt_tcp_connect_retries();

		/** @brief 00 80 80 00 03 01
		 *
		 * @return true if SSL is configured
		 */
		bool has_ipt_ssl();

		/**
		 * connecting to the IP-T master could be disabled
		 */
		bool is_ipt_enabled() const;

		/**
		 * extract IP-T configuration
		 */
		ipt::redundancy get_ipt_redundancy();
		ipt::master_record get_ipt_master();

		/**
		 * get current IP-T master
		 */
		std::uint8_t get_ipt_master_index();

		/**
		 * get current IP-T scramble key
		 */
		ipt::scramble_key get_ipt_sk();

		/**
		 * Switch to other IP-T redundancy
		 * Currently only 2 redundancies supported, so the only viable
		 * strategy is a round-robin algorithm.
		 *
		 * @return new master config record
		 */
		ipt::master_record switch_ipt_redundancy();

	private:
		/**
		 * extract IP-T host configuration
		 * @param idx only the values 1 and 2 are valid
		 */
		std::string get_ipt_host(std::uint8_t idx);
		std::uint16_t get_ipt_port_target(std::uint8_t idx);
		std::uint16_t get_ipt_port_source(std::uint8_t idx);
		std::string get_ipt_user(std::uint8_t idx);
		std::string get_ipt_pwd(std::uint8_t idx);
		bool is_ipt_scrambled(std::uint8_t idx);
		ipt::scramble_key get_ipt_sk(std::uint8_t idx);

		/**
		 * extract IP-T configuration
		 * @param idx only the values 1 and 2 are valid
		 */
		ipt::master_record get_ipt_cfg(std::uint8_t idx);

	private:
		cache& cache_;
	};


}
#endif
