/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_DB_SCHEMES_H
#define NODE_DB_SCHEMES_H

namespace cyng {
	namespace store {
		class db;
	}
}
namespace node 
{
	bool create_table_device(cyng::store::db&);
	bool create_table_lora_device(cyng::store::db&);

	bool create_table_session(cyng::store::db&);
	bool create_table_target(cyng::store::db&);
	bool create_table_meter(cyng::store::db&);
	bool create_table_leased_line(cyng::store::db&);	
	bool create_table_connection(cyng::store::db&);
	bool create_table_cluster(cyng::store::db&);
	bool create_table_config(cyng::store::db&);
	bool create_table_sys_msg(cyng::store::db&);
	bool create_table_csv(cyng::store::db&);

	/**
	 * dash/s uses a different scheme
	 */
	bool create_table_gateway(cyng::store::db&);
}

#endif

