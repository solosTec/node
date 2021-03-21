/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/config/kv_store.h>

#include <cyng/obj/container_cast.hpp>
 
#ifdef _DEBUG_SEGW
#include <iostream>
#endif

#include <boost/uuid/nil_generator.hpp>

namespace smf {
	kv_store::kv_store(cyng::store& cache, boost::uuids::uuid tag)
		: cache_(cache)
		, tag_(tag)
	{}


	cyng::object kv_store::get_obj(std::string path) {
		return cache_.get_object("config", "val", path);
	}

	bool kv_store::set_obj(std::string name, cyng::object&& obj) {

		bool r = false;
		cache_.access([&](cyng::table* kv_store) {

			r = kv_store->merge(cyng::key_generator(name)
				, cyng::data_generator(obj)
				, 1u	//	only needed for insert operations
				, tag_);	//	tag not available yet

		}, cyng::access::write("config"));
		return r;
	}

	bool kv_store::set_value(std::string name, cyng::object obj) {
		return set_obj(name, std::move(obj));
	}

	boost::uuids::uuid kv_store::get_tag() const {
		return tag_;
	}

}