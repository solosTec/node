/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_CLI_SEND_TCP_IP
#define SMF_CLI_SEND_TCP_IP

#include <cyng/obj/intrinsics/buffer.h>

#include <string> // for to_string, allocator

/**
 * send synchrous
 */
bool send_tcp_ip(std::string host, std::string service, std::string path);

#endif
