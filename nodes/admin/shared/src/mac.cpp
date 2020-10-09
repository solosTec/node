/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "mac.h"
#include "oui.h"
#include <cyng/io/io_buffer.h>

namespace node
{
    std::string lookup_org(cyng::mac48 const& m)
    {
        auto const org = m.get_oui();
        cyng::buffer_t const buf(org.begin(), org.end());
        auto const key = cyng::io::to_hex(buf);
        auto const pos = oui.find(key);
        return (pos != oui.end())
            ? pos->second
            : ""
            ;
    }

}
