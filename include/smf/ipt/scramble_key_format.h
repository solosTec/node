/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_SCRAMBLE_KEY_FORMAT_H
#define SMF_IPT_SCRAMBLE_KEY_FORMAT_H

#include <smf/ipt/scramble_key_io.hpp>
#include <ostream>
#include <boost/uuid/string_generator.hpp>

namespace smf
{
	namespace ipt	
	{
		/**
		 * Fixed length is 64 characters
		 * 
		 * @return scramble key as hex string
		 */
		std::string to_string(scramble_key const&);

		scramble_key to_sk(std::string const&);

	}	//	ipt
}


#endif
