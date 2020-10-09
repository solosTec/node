/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_ADMIN_MAC_H
#define NODE_ADMIN_MAC_H

#include <string>
#include <cyng/intrinsics/mac.h>

namespace node
{
    std::string lookup_org(cyng::mac48 const&);

}

#endif