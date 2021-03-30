/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_CONFIG_KV_STORE_H
#define SMF_CONFIG_KV_STORE_H


#include <cyng/log/logger.h>
#include <cyng/store/db.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/value_cast.hpp>

#include <boost/uuid/uuid.hpp>

namespace smf {

	/**
	 * manage configuration data
	 */
	class kv_store
	{
	public:
		kv_store(cyng::store&, boost::uuids::uuid tag);

		/**
		 * read a configuration object from table "kv_store"
		 */
		cyng::object get_obj(std::string name);

		/**
		 * read a configuration value from table "_Cfg"
		 */
		template <typename T >
		T get_value(std::string name, T def) {
			return cyng::value_cast(get_obj(name), def);
		}

		/**
		 * The non-template function wins.
		 */
		std::string get_value(std::string name, const char* def) {
			return cyng::value_cast(get_obj(name), std::string(def));
		}

		/**
		 * set/insert a configuration value
		 */
		bool set_obj(std::string name, cyng::object&& obj);
		bool set_value(std::string name, cyng::object obj);

		template <typename T >
		bool set_value(std::string name, T value) {
			return set_obj(name, cyng::make_object(std::move(value)));
		}

		/**
		 * @return itentity/source tag
		 */
		boost::uuids::uuid get_tag() const;

	private:
		cyng::store& cache_;
		boost::uuids::uuid const tag_;
	};
}

#endif