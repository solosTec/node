/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include <smf/cluster/generator.h>
#include <cyng/chrono.h>
#include <cyng/intrinsics/label.h>

namespace node
{
	cyng::vector_t bus_req_login(std::string const& host
		, std::string const& service
		, std::string const& account
		, std::string const& pwd
		, std::string const& node_class)
	{
		cyng::vector_t prg;
		return prg
			<< cyng::generate_invoke("ip.tcp.socket.resolve", host, service)
			<< ":SEND-LOGIN-REQUEST"			//	label
			<< cyng::code::JNE					//	jump if no error
			<< cyng::generate_invoke("bus.reconfigure", cyng::code::LERR)
			<< cyng::generate_invoke("log.msg.error", cyng::code::LERR)	// load error register
			<< ":STOP"							//	label
			<< cyng::code::JA					//	jump always
			<< cyng::label(":SEND-LOGIN-REQUEST")
			<< cyng::generate_invoke("bus.start")		//	start reading cluster bus
			<< cyng::generate_invoke("stream.serialize"
				, cyng::generate_invoke_remote("bus.req.login"
					, account
					, pwd
					, cyng::code::IDENT
					, node_class
					, cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR)
					, cyng::make_object(cyng::chrono::delta())
					, cyng::make_object(std::chrono::system_clock::now())))
			<< cyng::generate_invoke("stream.flush")
			<< cyng::label(":STOP")
			<< cyng::code::NOOP
			<< cyng::reloc()
			;

	}

	cyng::vector_t bus_shutdown()
	{
		cyng::vector_t prg;
		return prg
			<< cyng::generate_invoke("log.msg.warning", "shutdown cluster member", cyng::IDENT)
			<< cyng::generate_invoke("ip.tcp.socket.shutdown")
			<< cyng::generate_invoke("ip.tcp.socket.close")
			;
	}

	cyng::vector_t bus_req_subscribe(std::string const& table)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"

			//	bounce back 
			, cyng::generate_invoke_remote("stream.serialize", cyng::generate_invoke_reflect("db.trx.start"))
			, cyng::generate_invoke_remote("stream.flush")

			, cyng::generate_invoke_remote("bus.req.subscribe", table, cyng::IDENT)

			//	bounce back 
			, cyng::generate_invoke_remote("stream.serialize", cyng::generate_invoke_reflect("db.trx.commit"))
			, cyng::generate_invoke_remote("stream.flush")
			)

		<< cyng::generate_invoke("stream.flush")
		;
	}

	cyng::vector_t bus_req_unsubscribe(std::string const& table)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"
			, cyng::generate_invoke_remote("bus.req.unsubscribe", table, cyng::IDENT))
			<< cyng::generate_invoke("stream.flush")
			;
	}

	cyng::vector_t bus_db_insert(std::string const& table
		, cyng::vector_t const& key
		, cyng::vector_t const& data
		, std::uint64_t generation)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"
			, cyng::generate_invoke_remote("db.insert", table, key, data, generation, cyng::IDENT))
			<< cyng::generate_invoke("stream.flush")
			;
	}

	cyng::vector_t client_req_login(boost::uuids::uuid tag
		, std::string const& name
		, std::string const& pwd
		, std::string const& scheme //	login scheme
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"
			, cyng::generate_invoke_remote("client.req.login", tag, cyng::IDENT, cyng::invoke("bus.seq.next"), name, pwd, scheme, bag))
			<< cyng::generate_invoke("stream.flush")
			;
	}

	cyng::vector_t client_req_open_push_channel(boost::uuids::uuid tag
		, std::string const& target
		, std::string const& device
		, std::string const& number
		, std::string const& version
		, std::string const& id
		, std::uint16_t timeout
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"
			, cyng::generate_invoke_remote("client.req.open.push.channel", tag, cyng::IDENT, cyng::invoke("bus.seq.next"), target, device, number, version, id, timeout, bag))
			<< cyng::generate_invoke("stream.flush")
			;
	}

	cyng::vector_t client_res_login(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, std::string const& msg
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"
			, cyng::generate_invoke_remote("client.res.login", tag, cyng::IDENT, seq, success, msg, bag))
			<< cyng::generate_invoke("stream.flush")
			;
	}


}
