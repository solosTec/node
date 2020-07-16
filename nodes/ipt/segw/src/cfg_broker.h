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
		cfg_broker(cache&);

		/**
		 * @param tty index to select port (ttyAPP[N])
		 */
		bool is_transparent_mode(std::uint8_t tty) const;

	private:
		cache& cache_;
	};
}
#endif
