/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/ipt/bus.h>
#include <cyng/vm/mesh.h>

namespace smf
{
	namespace ipt
	{
		bus::bus(cyng::mesh& mesh, toggle::server_vec_t const& tgl)
			: mesh_(mesh)
			, tgl_(tgl)
		{}

	}	//	ipt
}



