/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#include <smf/iec/parser.h>
#include <cyng/vm/generator.h>
#include <boost/algorithm/string.hpp>

namespace node	
{
	namespace iec
	{
		/*
		 *	IEC 62056-21 parser.
		 */
		parser::parser(parser_callback cb)
			: cb_(cb)
			, code_()
		{
		}

		parser::~parser()
		{}

		void parser::parse(char c)	
		{
		}


		void parser::post_processing()	
		{
		}

	}
}	

