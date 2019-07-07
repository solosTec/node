/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#include <smf/http/srv/generator.h>

namespace node 
{
	namespace db 
	{ 
		cyng::vector_t insert(std::string const& table
			, cyng::vector_t key
			, cyng::vector_t body
			, std::uint64_t generation
			, boost::uuids::uuid source)
		{
			std::reverse(key.begin(), key.end());
			std::reverse(body.begin(), body.end());
			return cyng::generate_invoke("db.req.insert"
				, table
				, key
				, body
				, generation
				, source);
		}

		cyng::vector_t modify_by_param(std::string const&
			, cyng::vector_t key
			, cyng::param_t && param
			, std::uint64_t generation
			, boost::uuids::uuid source)
		{
			std::reverse(key.begin(), key.end());
			return cyng::generate_invoke("db.req.modify.by.param"
				, "_HTTPSession"
				, key
				, std::move(param)
				, generation
				, source);
		}

		cyng::vector_t remove(std::string const& table
			, cyng::vector_t key
			, boost::uuids::uuid source)
		{
			std::reverse(key.begin(), key.end());
			return cyng::generate_invoke("db.req.remove"
				, "_HTTPSession"
				, key
				, source);
		}

		cyng::vector_t clear(std::string const& table)
		{
			return cyng::generate_invoke("db.clear", table);
		}

	}
}

