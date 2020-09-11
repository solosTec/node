/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_LIB_HTTP_COUNTER_H
#define NODE_LIB_HTTP_COUNTER_H

#include <boost/asio.hpp>
#include <map>

namespace node 
{
	class connection_counter
	{
	public:
		connection_counter();
		std::size_t insert(boost::asio::ip::address const&);
		std::size_t size() const;
	private:
		std::map<boost::asio::ip::address, std::size_t>  counter_;

	};
}

#endif
