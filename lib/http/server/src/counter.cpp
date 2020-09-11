/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#include <smf/http/srv/counter.h>

namespace node 
{
	connection_counter::connection_counter()
		: counter_()
	{}

	std::size_t connection_counter::insert(boost::asio::ip::address const& address)
	{
		auto pos = counter_.find(address);
		if (pos != counter_.end()) {
			return ++(pos->second);
		}
		counter_.emplace(address, 1);
		return 1u;
	}

	std::size_t connection_counter::size() const
	{
		return counter_.size();
	}

}
