/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "session.h"
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
	namespace sml
	{
		session::session(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cyng::store::db& config_db
			, node::ipt::redundancy const& cfg
			, std::string const& account
			, std::string const& pwd
			, bool accept_all)
		: mux_(mux)
			, logger_(logger)
			, vm_(mux.get_io_service(), boost::uuids::random_generator()())
			, parser_([this](cyng::vector_t&& prg) {
				CYNG_LOG_INFO(logger_, prg.size() << " instructions received");
				CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
				vm_.async_run(std::move(prg));
			}, false, false, false)
			, core_(logger_
				, vm_
				, config_db
				, cfg
				, true	//	server mode
				, account
				, pwd
				, accept_all)	//	check credendials
		{
			//
			//	this external interface
			//	atomic status update
			//
			config_db.access([&](cyng::store::table* tbl) {

				auto word = cyng::value_cast<std::uint64_t>(get_config(tbl, OBIS_CLASS_OP_LOG_STATUS_WORD.to_str()), 0u);
				CYNG_LOG_DEBUG(logger_, "status word: " << word);
				status status(word);

				status.set_ext_if_available(true);
				update_config(tbl, OBIS_CLASS_OP_LOG_STATUS_WORD.to_str(), word, vm_.tag());

			}, cyng::store::write_access("_Config"));


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
			, cyng::store::db& config_db
			, node::ipt::redundancy const& cfg
			, std::string const& account
			, std::string const& pwd
			, bool accept_all)
		{
			return cyng::make_object<session>(mux, logger, config_db, cfg, account, pwd, accept_all);
		}

	}
}

namespace cyng
{
	namespace traits
	{

#if !defined(__CPP_SUPPORT_N2235)
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

	bool less<node::sml::session>::operator()(node::sml::session const& s1, node::sml::session const& s2) const noexcept
	{
		return s1.hash() < s2.hash();
	}

}