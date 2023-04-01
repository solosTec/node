/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_CLI_ANALYZE_LOG_V8_H
#define SMF_CLI_ANALYZE_LOG_V8_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory> // for shared_ptr, __shared_ptr_access
#include <string> // for to_string, allocator

/**
 * Extract push messages from IP-T log file
 */
void analyse_log_v8(std::string s);

#endif
