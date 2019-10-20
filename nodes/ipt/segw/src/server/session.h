/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_SESSION_H
#define NODE_SEGW_SESSION_H

#include <smf/sml/protocol/parser.h>
#include <smf/sml/protocol/reader.h>
//#include "core.h"
#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/vm/controller.h>

namespace node
{
	class connection;
	class cache;
	class session
	{
		friend class connection;
	public:
		session(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cache& config_db
			, std::string const& account
			, std::string const& pwd
			, bool accept_all);

		session(session const&) = delete;
		session& operator=(session const&) = delete;

		/**
			* @return session specific hash based in internal tag
			*/
		std::size_t hash() const noexcept;


	private:
		cyng::async::mux& mux_;
		cyng::logging::log_ptr logger_;
		cyng::controller vm_;

		/**
		 * SML parser
		 */
		sml::parser 	parser_;

		/**
		 * Provide core functions of an SML gateway
		 */
		//sml::kernel core_;

	};

	cyng::object make_session(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cache& config_db
		, std::string const& account
		, std::string const& pwd
		, bool accept_all);
}

#include <cyng/intrinsics/traits.hpp>

namespace cyng
{
	namespace traits
	{
		template <>
		struct type_tag<node::session>
		{
			using type = node::session;
			using tag = std::integral_constant<std::size_t,
#if defined(__CPP_SUPPORT_N2347)
				static_cast<std::size_t>(traits::predef_type_code::PREDEF_CUSTOM) + 1
#else
				PREDEF_CUSTOM + 1
#endif
			>;

#if defined(__CPP_SUPPORT_N2235)
			constexpr static char name[] = "session";
#else
			const static char name[];
#endif
		};

		template <>
		struct reverse_type <
#if defined(__CPP_SUPPORT_N2347)
			static_cast<std::size_t>(traits::predef_type_code::PREDEF_CUSTOM) + 1
#else
			PREDEF_CUSTOM + 1
#endif
		>
		{
			using type = node::session;
		};
	}
}

#include <functional>
#include <boost/functional/hash.hpp>

namespace std
{
	template<>
	struct hash<node::session>
	{
		size_t operator()(node::session const& s) const noexcept;
	};
	template<>
	struct equal_to<node::session>
	{
		using result_type = bool;
		using first_argument_type = node::session;
		using second_argument_type = node::session;

		bool operator()(node::session const& s1, node::session const& s2) const noexcept;
	};
	template<>
	struct less<node::session>
	{
		using result_type = bool;
		using first_argument_type = node::session;
		using second_argument_type = node::session;

		bool operator()(node::session const& s1, node::session const& s2) const noexcept;
	};
}


#endif