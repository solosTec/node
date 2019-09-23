/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_TOOL_SMF_CLI_H
#define NODE_TOOL_SMF_CLI_H

#include "console.h"
#include "convert.h"
#include <cyng/compatibility/io_service.h>

namespace node
{
	class cli : public console
	{
		friend class convert;
	public:
		cli(cyng::io_service_t&, boost::uuids::uuid, std::ostream&, std::istream&);
		virtual ~cli();

		int run();
		int run(std::string const& file_name);

	protected:
		virtual bool parse(std::string const&);
		void call(std::string, std::vector<std::string> const&);

	private:
		convert	plugin_convert_;
	};
}

#endif	
