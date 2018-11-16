/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_DB_META_H
#define NODE_DB_META_H

#include <cyng/table/meta_interface.h>

namespace node 
{
	cyng::table::meta_table_ptr TSMLMeta();
	cyng::table::meta_table_ptr TSMLData();

	cyng::table::meta_table_ptr gw_devices();
	cyng::table::meta_table_ptr gw_push_ops();
	cyng::table::meta_table_ptr gw_op_log();

}

#endif

