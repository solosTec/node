/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/http/auth.h>

#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/value_cast.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/parse/string.h>

#include <smfsec/hash/base64.h>

#include <boost/algorithm/string.hpp>

namespace smf {

	namespace http {

		auth::auth()
			: type_()
			, user_()
			, pwd_()
			, realm_()
		{}

		auth::auth(std::string const& type, std::string const& user, std::string const& pwd, std::string const& realm)
			: type_(type)
			, user_(user)
			, pwd_(pwd)
			, realm_(realm)
		{}

		std::string auth::basic_credentials() const
		{
			return cyng::crypto::base64_encode(user_ + ":" + pwd_);
		}

		std::string const& auth::get_user() const {
			return user_;
		}


		auth to_auth(cyng::param_map_t pmap) {

			//	"directory", "authType", "realm", "name" and "pwd"
			auto const reader = cyng::make_reader(pmap);
			return auth(
				value_cast(reader["authType"].get(), "Basic"),
				value_cast(reader["name"].get(), ""),
				value_cast(reader["pwd"].get(), ""),
				value_cast(reader["realm"].get(), "realm")
			);
		}

		auth_dirs to_auth_dirs(cyng::vector_t vec) {

			auth_dirs r;
			std::transform(vec.begin(), vec.end(), std::inserter(r, r.end()), [](cyng::object const& obj) {

				auto const reader = cyng::make_reader(obj);
				auto const dir = value_cast(reader["directory"].get(), "/");

				return std::make_pair(dir, to_auth(cyng::container_cast<cyng::param_map_t>(obj)));

				});
			return r;

		}

		bool is_auth_required(std::string const& path, auth_dirs const& ad) {
			for (auto const& dir : ad) 	{

				if (boost::algorithm::starts_with(path, dir.first)) return true;

			}
			return false;

		}

		std::pair<auth, bool> is_credentials_valid(boost::string_view path, auth_dirs const& ad) {

			std::vector<boost::string_view> parts = cyng::split(path, "\t ");
			BOOST_ASSERT_MSG(parts.size() == 2, "invalid authorization field");
			if (parts.size() == 2 && (parts.at(0) == "Basic")) {
				for (auto ident : ad) {
					if (ident.second.basic_credentials() == parts.at(1)) {
						return std::make_pair(ident.second, true);
					}
				}
			}

			return std::make_pair(auth(), false);
		}

	}
}
