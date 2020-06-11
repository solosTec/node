/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 
#include "test-mbus-005.h"
#include <NODE_project_info.h>
#include <boost/test/unit_test.hpp>
//C:\projects\node\nodes\ipt\segw\src\decoder.h


namespace node 
{
	bool test_mbus_005()
	{
		//mbus.long.frame - [438a8867-0be9-4cd1-83a4-cd6295ca6ee0,false,8,9,72,a9,55010019E61E3C07380000000C78550100190C1326000000]
		//	C-field 8: RSP_UD
		//	A-field 9: primary address
		//	CI-field: 72: application layer
		return true;
	}

}
