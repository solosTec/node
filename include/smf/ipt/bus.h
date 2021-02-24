/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_BUS_H
#define SMF_IPT_BUS_H

#include <smf/ipt/config.h>

namespace cyng {
	class mesh;
}

namespace smf
{
	namespace ipt	
	{
		class bus
		{
		public:
			bus(cyng::mesh& mesh, toggle::server_vec_t const&);
		private:
			cyng::mesh& mesh_;
			toggle::server_vec_t tgl_;
		};
	}	//	ipt
}


#endif
