/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_CLI_TRANSFER_TO_DB_H
#define SMF_CLI_TRANSFER_TO_DB_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory> // for shared_ptr, __shared_ptr_access
#include <string> // for to_string, allocator

/**
 * Read a list of stored push data and transfer content to database
 */
void transfer_to_db(std::string s, std::string p);

#endif
