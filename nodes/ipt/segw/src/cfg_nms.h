/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2021 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_BROKER_SEGW_NMS_H
#define NODE_BROKER_SEGW_NMS_H

#include "segw.h"
#include <boost/asio.hpp>
 
namespace node
{

	class cache;

	/**
	 * NMS
	 * ROOT_NMS:NMS_ADDRESS......................: 0.0.0.0 (0.0.0.0:ip:address)
	 * ROOT_NMS:NMS_PORT.........................: [07d0/1c5d] (07d0:u16)      - 2000 (decimal)
	 * ROOT_NMS:NMS_USER.........................: operator (operator:s)
	 * ROOT_NMS:NMS_PWD..........................: operator (operator:s)
	 * ROOT_NMS:NMS_ENABLED......................: true (true:bool)
	 * ROOT_NMS:script-path......................: C:\Users\Sylko\AppData\Local\Temp\update-script.cmd (C:\Users\Sylko\AppData\Local\Temp\update-script.cmd:s)
	 */
	class cfg_nms
	{
	public:
		cfg_nms(cache&);

		boost::asio::ip::tcp::endpoint get_ep() const;
		boost::asio::ip::address get_address() const;
		std::uint16_t get_port() const;
		std::string get_user() const;
		std::string get_pwd() const;
		bool is_enabled() const;


		bool set_address(boost::asio::ip::address) const;
		bool set_port(std::uint16_t) const;
		bool set_user(std::string) const;
		bool set_pwd(std::string) const;
		bool set_enabled(bool) const;

	private:
		cache& cache_;
	};

}
#endif
