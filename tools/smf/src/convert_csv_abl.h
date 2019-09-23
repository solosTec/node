/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_TOOL_SMF_PLUGIN_CONVERT_H
#define NODE_TOOL_SMF_PLUGIN_CONVERT_H

//#include "console.h"
//#include <cyng/compatibility/io_service.h>

namespace node
{
	class cli;
	class convert
	{
	public:
		convert(cli*);
		virtual ~convert();


	private:
		cli* cli_;
	};
}

#endif	
