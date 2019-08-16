/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "write_pid.h"
#include <cyng/sys/process.h>
#include <fstream>
#include <boost/uuid/uuid_io.hpp>

namespace node 
{
	bool write_pid(boost::filesystem::path const& log_dir, boost::uuids::uuid tag)
	{
		const auto pid = (log_dir / boost::uuids::to_string(tag)).replace_extension(".pid").string();
		std::fstream fout(pid, std::ios::trunc | std::ios::out);
		if (fout.is_open())
		{
			fout << cyng::sys::get_process_id();
			fout.close();
			return true;
		}
		return false;
	}
}

