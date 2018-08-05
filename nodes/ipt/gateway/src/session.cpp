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
			, status& status_word
			, cyng::store::db& config_db
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
			}, false, false)
			, core_(logger_, vm_, status_word, config_db, true, account, pwd, manufacturer, model, mac)
			//, task_db_()
		{
			//
			//	this external interface
			//
			core_.status_word_.set_ext_if_available(true);

			//vm_.register_function("session.store.relation", 2, std::bind(&session::store_relation, this, std::placeholders::_1));

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

		//void session::store_relation(cyng::context& ctx)
		//{
		//	//	[1,2]
		//	//
		//	//	* ipt sequence number
		//	//	* task id
		//	//	
		//	const cyng::vector_t frame = ctx.get_frame();
		//	CYNG_LOG_INFO(logger_, "session.store.relation " << cyng::io::to_str(frame));

		//	task_db_.emplace(cyng::value_cast<node::ipt::sequence_type>(frame.at(0), 0), cyng::value_cast<std::size_t>(frame.at(1), 0));
		//}

		cyng::object make_session(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, status& status_word
			, cyng::store::db& config_db
			, std::string const& account
			, std::string const& pwd
			, std::string manufacturer
			, std::string model
			, cyng::mac48 mac)
		{
			return cyng::make_object<session>(mux, logger, status_word, config_db, account, pwd, manufacturer, model, mac);
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