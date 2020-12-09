/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_SEGW_GPIO_CONFIG_H
#define NODE_SEGW_GPIO_CONFIG_H

#include <cyng/compatibility/file_system.hpp>

#include <cstdint>
#include <vector>

namespace node
{
	/**
	 * manage GPIO configuration
	 */
	class cache;
	class cfg_gpio
	{
	public:
		cfg_gpio(cache&);

		bool is_enabled() const;

		/**
		 * @return a list of all GPIO IDs
		 */
		std::vector<std::uint32_t> get_vector() const;

		/**
		 * set/update task id that manages a single pin
		 */
		bool set_task(std::uint32_t pin, std::size_t tsk);

		std::size_t get_task(std::uint32_t pin);

		cyng::filesystem::path get_path() const;

		cyng::filesystem::path get_path(std::uint32_t pin) const;
	
	private:
		cache& cache_;
	};


}
#endif
