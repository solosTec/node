/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#include <cache.h>

#include <cyng/value_cast.hpp>

#include <boost/algorithm/string.hpp>

namespace node 
{

	cache::cache(cyng::logging::log_ptr logger, cyng::store::db& db)
		: logger_(logger)
		, db_(db)
	{}

	cyng::table::record cache::lookup_meter_by_ident(std::string const& ident)
	{
		return db_.access([&](cyng::store::table const* tbl) -> cyng::table::record {

			auto result = cyng::table::make_empty_record(tbl->meta_ptr());
			const auto counter = tbl->loop([&](cyng::table::record const& rec) -> bool {

				auto const str = cyng::value_cast(rec["ident"], "00000000");
				if (boost::algorithm::equals(ident, str)) {
#ifdef _DEBUG
					CYNG_LOG_DEBUG(logger_, "meter ident " 
						<< ident 
						<< " found: "
						<< cyng::io::to_type(rec.convert()));
#endif
					result = rec;
					return false;
				}

				//	continue
				return true;
				});

			return result;

		}, cyng::store::read_access("TMeter"));
	}

	cyng::crypto::aes_128_key cache::lookup_aes(cyng::table::key_type const& key)
	{
		auto const rec = db_.lookup("TMeterAccess", key);
		if (!rec.empty()) {
			return cyng::value_cast(rec["aes"], cyng::crypto::aes_128_key());
		}
		return cyng::crypto::aes_128_key();
	}

}
