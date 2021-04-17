/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_DASH_HTTP_DB_H
#define SMF_DASH_HTTP_DB_H

#include <smf/config/schemes.h>
#include <smf/config/kv_store.h>

#include <cyng/store/db.h>
#include <cyng/log/logger.h>
#include <cyng/obj/numeric_cast.hpp>

#include <boost/uuid/random_generator.hpp>

namespace smf {

	class notifier;
	class db
	{
		friend class notifier;
	public:
		/**
		 * releation between table name, channel name and table size channel
		 */
		struct rel {
			rel(std::string, std::string, std::string);
			rel() = default;
			rel(rel const&) = default;

			bool empty() const;

			std::string const table_;
			std::string const channel_;
			std::string const counter_;
		};

	public:
		db(cyng::store& cache
			, cyng::logger
			, boost::uuids::uuid tag);

		void init(std::uint64_t max_upload_size
			, std::string const& nickname
			, std::chrono::seconds timeout
			, std::string const& country_code
			, std::string const& lang_code
			, cyng::slot_ptr);

		void res_insert(std::string table_name
			, cyng::key_t  key
			, cyng::data_t  data
			, std::uint64_t gen
			, boost::uuids::uuid tag);

		void res_update(std::string table_name
			, cyng::key_t key
			, cyng::attr_t attr
			, std::uint64_t gen
			, boost::uuids::uuid tag);

		void res_remove(std::string table_name
			, cyng::key_t  key
			, boost::uuids::uuid tag);

		void res_clear(std::string table_name
			, boost::uuids::uuid tag);

		/**
		 * search a relation record by table name
		 */
		rel by_table(std::string const& name) const;

		/**
		 * search a relation record by channel name
		 */
		rel by_channel(std::string const& name) const;

		/**
		 * search a relation record by counter name
		 */
		rel by_counter(std::string const& name) const;

		/**
		 * loop over all rel-entries
		 */
		void loop_rel(std::function<void(rel const&)>) const;

		void add_to_subscriptions(std::string const& channel, boost::uuids::uuid tag);

		/**
		 * remove (a ws) from the subscription list
		 *
		 * @return number of affected subscriptions
		 */
		std::size_t remove_from_subscription(boost::uuids::uuid tag);

		/**
		 * convert data types from ws to matching data types
		 * in table schemes.
		 */
		void convert(std::string const& table_name
			, cyng::vector_t& key
			, cyng::param_map_t& data);

		/**
		 * convert data types from ws to matching data types
		 * in table schemes.
		 */
		void convert(std::string const& table_name
			, cyng::vector_t& key);

		/**
		 * To insert a new entity over the web interface a new primary key
		 * for the database is required.
		 */
		cyng::vector_t generate_new_key(std::string const&, cyng::vector_t&& key, cyng::param_map_t const& data);

		cyng::data_t complete(std::string const&, cyng::param_map_t&&);

		/**
		 * Search a meter record by it's meter id
		 */
		cyng::key_t lookup_meter_by_id(std::string const&);

	private:
		void set_start_values(std::uint64_t max_upload_size
			, std::string const& nickname
			, std::chrono::seconds timeout
			, std::string const& country_code
			, std::string const& lang_code);

		/**
		 * @return object of the specified type with a generic NULL value
		 */
		cyng::object generate_empty_value(cyng::type_code);

	public:
		kv_store cfg_;
		cyng::store& cache_;

	private:
		cyng::logger logger_;
		config::store_map store_map_;

		using array_t = std::array<rel, 16>;
		static array_t const rel_;

		/** @brief channel list
		 *
		 * Each channel can be subscribed by a multitude of channels
		 * channel => ws
		 */
		std::multimap<std::string, boost::uuids::uuid>	subscriptions_;

		/**
		 * generate unique tags
		 */
		boost::uuids::random_generator uidgen_;

	};

	/**
	 * @return vector of all required meta data
	 */
	std::vector< cyng::meta_store > get_store_meta_data();

	cyng::object convert_to_type(cyng::type_code, cyng::object&);
	cyng::object convert_to_uuid(cyng::object&);
	cyng::object convert_to_tp(cyng::object&);
	cyng::object convert_to_ip_address(cyng::object&);
	cyng::object convert_to_aes128(cyng::object&);
	cyng::object convert_to_aes192(cyng::object& obj);
	cyng::object convert_to_aes256(cyng::object& obj);

	template <typename T>
	cyng::object convert_to_numeric(cyng::object& obj) {
		if (obj.rtti().tag() == cyng::TC_NULL)	return T(0);
		BOOST_ASSERT(obj.rtti().is_integral());
		return cyng::make_object(cyng::numeric_cast<T>(obj, T(0)));
	}

	cyng::object convert_to_nanoseconds(cyng::object&);
	cyng::object convert_to_microseconds(cyng::object&);
	cyng::object convert_to_milliseconds(cyng::object&);
	cyng::object convert_to_seconds(cyng::object&);
	cyng::object convert_to_minutes(cyng::object&);
	cyng::object convert_to_hours(cyng::object&);

}

#endif
