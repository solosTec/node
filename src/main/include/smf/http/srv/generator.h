/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_HTTP_GENERATOR_H
#define NODE_HTTP_GENERATOR_H

#include <cyng/intrinsics/sets.h>
#include <cyng/vm/generator.h>

namespace node 
{
	namespace db
	{
		/**
		 * database operations
		 */
		cyng::vector_t insert(std::string const&
			, cyng::vector_t 
			, cyng::vector_t 
			, std::uint64_t generation
			, boost::uuids::uuid source);

		cyng::vector_t modify_by_param(std::string const&
			, cyng::vector_t
			, cyng::param_t&&
			, std::uint64_t generation
			, boost::uuids::uuid source);

		//cyng::vector_t insert(std::string const&
		//	, cyng::vector_t const&
		//	, cyng::vector_t const&
		//	, std::uint64_t generation);

		//cyng::vector_t merge(std::string const&
		//	, cyng::vector_t const&
		//	, cyng::vector_t const&
		//	, std::uint64_t generation
		//	, boost::uuids::uuid source);

		//cyng::vector_t update(std::string const&
		//	, cyng::vector_t const&
		//	, cyng::vector_t const&
		//	, std::uint64_t generation
		//	, boost::uuids::uuid source);

		//cyng::vector_t modify(std::string const&
		//	, cyng::vector_t const&
		//	, cyng::attr_t const&
		//	, boost::uuids::uuid source);

		//cyng::vector_t modify(std::string const&
		//	, cyng::vector_t const&
		//	, cyng::param_t const&
		//	, std::uint64_t gen
		//	, boost::uuids::uuid source);

		cyng::vector_t remove(std::string const&
			, cyng::vector_t
			, boost::uuids::uuid source);

		cyng::vector_t clear(std::string const&);
	}
}

#endif
