/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "session.h"
#include "../cache.h"
#include <smf/shared/db_cfg.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/status.h>

#include <cyng/vm/domain/log_domain.h>
#include <cyng/io/serializer.h>
#include <cyng/tuple_cast.hpp>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace node
{
	session::session(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cache& cfg
		, storage& db
		, std::string const& account
		, std::string const& pwd
		, bool accept_all)
	: mux_(mux)
		, logger_(logger)
		, vm_(mux.get_io_service(), boost::uuids::random_generator()())
		, parser_([this](cyng::vector_t&& prg) {
			CYNG_LOG_TRACE(logger_, prg.size() << " SML instructions received (server)");
			//CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
			vm_.async_run(std::move(prg));
		}, false, false, false)
		, router_(logger_
			, true	//	server mode
			, vm_
			, cfg
			, db
			, account
			, pwd
			, accept_all)
	{
		//
		//	register logger domain
		//
		cyng::register_logger(logger_, vm_);
		vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

		vm_.register_function("net.start.tsk.push", 8, [&](cyng::context& ctx) {

			//
			//	start <push> task
			//
			CYNG_LOG_WARNING(logger_, "cannot start <push> task "
				<< vm_.tag());

			auto const frame = ctx.get_frame();
			//CYNG_LOG_INFO(logger_, ctx.get_name()
			//	<< " - "
			//	<< cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				cyng::buffer_t,		//	[0] srv_id
				std::uint8_t,		//	[1] nr
				cyng::buffer_t,		//	[2] profile
				std::uint32_t,		//	[3] interval
				std::uint32_t,		//	[4] delay
				cyng::buffer_t,		//	[5] source
				std::string,		//	[6] target
				cyng::buffer_t		//	[7] service
			>(frame);

		});

		if (accept_all) {
			CYNG_LOG_WARNING(logger_, "new custom interface session "
				<< vm_.tag()
				<< " will accept all server IDs - this is a security risk");
		}
	}

	std::size_t session::hash() const noexcept
	{
		return vm_.hash();
	}


	cyng::object make_session(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cache& cfg
		, storage& db
		, std::string const& account
		, std::string const& pwd
		, bool accept_all)
	{
		return cyng::make_object<session>(mux, logger, cfg, db, account, pwd, accept_all);
	}
}

namespace cyng
{
	namespace traits
	{

#if !defined(__CPP_SUPPORT_N2235)
		const char type_tag<node::session>::name[] = "session";
#endif
	}	// traits	
}

namespace std
{
	size_t hash<node::session>::operator()(node::session const& s) const noexcept
	{
		return s.hash();
	}

	bool equal_to<node::session>::operator()(node::session const& s1, node::session const& s2) const noexcept
	{
		return s1.hash() == s2.hash();
	}

	bool less<node::session>::operator()(node::session const& s1, node::session const& s2) const noexcept
	{
		return s1.hash() < s2.hash();
	}

}