/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "show_fs_drives.h"
#include <cyng/sys/fsys.h>
#include <cyng/sys/info.h>
#include <cyng/io/serializer.h>

namespace node 
{
	int show_fs_drives(std::ostream& os)
	{
		const auto drives = cyng::sys::get_drive_names();
		for (auto const& d : drives) {
			std::cout << d << std::endl;
		}
		return EXIT_SUCCESS;
	}
}

