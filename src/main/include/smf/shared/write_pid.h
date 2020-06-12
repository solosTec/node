/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_WRITE_PID_H
#define NODE_WRITE_PID_H

#include <boost/uuid/uuid.hpp>
#include <cyng/compatibility/file_system.hpp>

namespace node 
{
        bool write_pid(cyng::filesystem::path const& log_dir, boost::uuids::uuid tag);
}

#endif
