/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "session.h"
#include <cyng/vm/domain/log_domain.h>
#include <cyng/io/serializer.h>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace node
{
	namespace sml
	{
		session::session(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, std::string const& account
			, std::string const& pwd
			, std::string manufacturer
			, std::string model
			, cyng::mac48 mac)
		: mux_(mux)
			, logger_(logger)
			, vm_(mux.get_io_service(), boost::uuids::random_generator()())
			, parser_([this](cyng::vector_t&& prg) {
				CYNG_LOG_INFO(logger_, prg.size() << " instructions received");
				CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
				vm_.async_run(std::move(prg));
			}, false)
			, core_(logger_, vm_, true, account, pwd, manufacturer, model, mac)
		{
			//
			//	register logger domain
			//
			cyng::register_logger(logger_, vm_);
			vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));
		}

		std::size_t session::hash() const noexcept
		{
			return vm_.hash();
		}

		cyng::object make_session(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, std::string const& account
			, std::string const& pwd
			, std::string manufacturer
			, std::string model
			, cyng::mac48 mac)
		{
			return cyng::make_object<session>(mux, logger, account, pwd, manufacturer, model, mac);
		}

	}
}

namespace cyng
{
	namespace traits
	{

#if defined(CYNG_LEGACY_MODE_ON)
		const char type_tag<node::sml::session>::name[] = "session";
#endif
	}	// traits	
}

namespace std
{
	size_t hash<node::sml::session>::operator()(node::sml::session const& s) const noexcept
	{
		return s.hash();
	}

	bool equal_to<node::sml::session>::operator()(node::sml::session const& s1, node::sml::session const& s2) const noexcept
	{
		return s1.hash() == s2.hash();
	}

}