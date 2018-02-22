/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_SCRAMBLE_KEY_FORMAT_H
#define NODE_IPT_SCRAMBLE_KEY_FORMAT_H

#include <smf/ipt/scramble_key_io.hpp>
#include <ostream>
#include <boost/uuid/string_generator.hpp>

namespace node
{
	namespace ipt	
	{
		/**
		 * Fixed length is 64 characters
		 * 
		 * @return scramble key as hex string
		 */
		std::string to_string(scramble_key const&);

		scramble_key from_string(std::string const&);

	}	//	ipt
}	//	node


#endif	//	NODE_IPT_SCRAMBLE_KEY_FORMAT_H
