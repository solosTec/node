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
			, std::string const& pwd)
		: mux_(mux)
			, logger_(logger)
			, vm_(mux.get_io_service(), boost::uuids::random_generator()())
			, parser_([this](cyng::vector_t&& prg) {
				CYNG_LOG_INFO(logger_, prg.size() << " instructions received");
				CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));
				vm_.async_run(std::move(prg));
			}, false)
			, account_(account)
			, pwd_(pwd)
			, reader_()
		{
			//
			//	register logger domain
			//
			cyng::register_logger(logger_, vm_);
			vm_.run(cyng::generate_invoke("log.msg.info", "log domain is running"));

			//	sml parser
			vm_.run(cyng::register_function("sml.msg", 1, std::bind(&session::sml_msg, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("sml.eom", 1, std::bind(&session::sml_eom, this, std::placeholders::_1)));

			//	sml reader
			vm_.run(cyng::register_function("sml.public.open.request", 8, std::bind(&session::sml_public_open_request, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("sml.public.close.request", 3, std::bind(&session::sml_public_close_request, this, std::placeholders::_1)));
			vm_.run(cyng::register_function("sml.get.proc.parameter.request", 8, std::bind(&session::sml_get_proc_parameter_request, this, std::placeholders::_1)));
		}

		std::size_t session::hash() const noexcept
		{
			return vm_.hash();
		}


		void session::sml_msg(cyng::context& ctx)
		{
			const cyng::vector_t frame = ctx.get_frame();

			//
			//	print sml message number
			//
			const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);
			CYNG_LOG_INFO(logger_, "XML processor sml.msg #"
				<< idx);

			//
			//	get message body
			//
			cyng::tuple_t msg;
			msg = cyng::value_cast(frame.at(0), msg);
			CYNG_LOG_INFO(logger_, "sml.msg " << cyng::io::to_str(frame));

			reader_.read(ctx, msg, idx);
		}


		void session::sml_eom(cyng::context& ctx)
		{
			//	[5213,3]
			//
			//	* CRC
			//	* message counter
			//
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.eom " << cyng::io::to_str(frame));

			const std::size_t idx = cyng::value_cast<std::size_t>(frame.at(1), 0);

			reader_.reset();
		}

		void session::sml_public_open_request(cyng::context& ctx)
		{
			//	[34481794-6866-4776-8789-6f914b4e34e7,180301091943374505-1,0,005056c00008,00:15:3b:02:23:b3,20180301092332,operator,operator]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* client ID - MAC from client
			//	* server ID - target server/gateway
			//	* request file id
			//	* username
			//	* password
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.public.open.request " << cyng::io::to_str(frame));

		}

		void session::sml_public_close_request(cyng::context& ctx)
		{
			//	
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.public.close.request " << cyng::io::to_str(frame));

		}

		void session::sml_get_proc_parameter_request(cyng::context& ctx)
		{
			//	[b5cfc8a0-0bf4-4afe-9598-ded99f71469c,180301094328243436-3,2,05:00:15:3b:02:23:b3,operator,operator,81 81 c7 82 01 ff,null]
			//
			//	* pk
			//	* transaction id
			//	* SML message id
			//	* server ID
			//	* username
			//	* password
			//	* OBIS (requested parameter)
			//	* attribute (should be null)
			const cyng::vector_t frame = ctx.get_frame();
			CYNG_LOG_INFO(logger_, "sml.get.proc.parameter.request " << cyng::io::to_str(frame));

			//CODE_ROOT_DEVICE_IDENT

		}

		cyng::object make_session(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, std::string const& account
			, std::string const& pwd)
		{
			return cyng::make_object<session>(mux, logger, account, pwd);
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