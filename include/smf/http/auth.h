/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_HTTP_AUTH_H
#define SMF_HTTP_AUTH_H

#include <cyng/obj/intrinsics/container.h>

#include <boost/utility/string_view.hpp>

namespace smf {
	namespace http {

		/**
		 * contains all authorization data
		 */
		class auth {
			//	directory: /
			//	authType:
			//	user:
			//	pwd:
		public:
			auth();
			auth(std::string const&, std::string const&, std::string const&, std::string const&);
			auth(auth const&) = default;

			/*
			 * Concatenate the user name and the password with a colon and generate
			 * the base64 encoded string.
			 */
			std::string basic_credentials() const;

			std::string const& get_user() const;

		private:
			std::string const type_;
			std::string const user_;
			std::string const pwd_;
			std::string const realm_;

		};

		/**
		 * Reads a parameter map with the entries:  "directory", "authType", "realm", "name" and "pwd" and
		 * and generates an auth object.
		 */
		auth to_auth(cyng::param_map_t);

		/**
		 * The same path can use multiple authorizations
		 */
		using auth_dirs = std::multimap<std::string, auth>;

		/**
		 * Convert a vector of objects that contains authorization data 
		 * into a vector of auth objects.
		 */
		auth_dirs to_auth_dirs(cyng::vector_t);

		/**
		 * checks if a path requires authorization
		 */
		bool is_auth_required(std::string const& path, auth_dirs const& ad);

		/**
		 * check credentials against authorization data
		 */
		std::pair<auth, bool> is_credentials_valid(boost::string_view, auth_dirs const&);


	}
}

#endif
