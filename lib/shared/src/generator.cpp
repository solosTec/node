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
		, bool auto_config
		, std::uint32_t group
		, std::string const& node_class)
	{
		cyng::vector_t prg;
		return prg
			<< cyng::generate_invoke("log.msg.debug", "resolve", host)	// debug
			<< cyng::generate_invoke("ip.tcp.socket.resolve", host, service)
			<< ":SEND-LOGIN-REQUEST"			//	label
			<< cyng::code::JNE					//	jump if no error
			<< cyng::generate_invoke("bus.reconfigure", cyng::code::LERR)
			<< cyng::generate_invoke("log.msg.error", cyng::code::LERR)	// load error register
			<< ":STOP"							//	label
			<< cyng::code::JA					//	jump always
			<< cyng::label(":SEND-LOGIN-REQUEST")
			<< cyng::generate_invoke("log.msg.debug", "starting cluster bus")	// debug
			<< cyng::generate_invoke("bus.start")		//	start reading cluster bus
			<< cyng::generate_invoke("stream.serialize"
				, cyng::generate_invoke_remote("bus.req.login"
					, cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR)
					, account
					, pwd
					, cyng::code::IDENT
					, node_class
					, cyng::make_object(cyng::chrono::delta())
					, cyng::make_now()
					, cyng::make_object(auto_config)
					, group
					, cyng::invoke_remote("ip.tcp.socket.ep.remote")
					, NODE_PLATFORM		//	since v0.4
					, cyng::code::PID	//	since v0.4
				))
			<< cyng::generate_invoke("stream.flush")
			<< cyng::label(":STOP")
			<< cyng::code::NOOP
			<< cyng::unwind_vec(47)
			<< cyng::reloc()
			;

	}

	cyng::vector_t bus_req_subscribe(std::string const& table, std::size_t tsk)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"

			//	bounce back 
			, cyng::generate_invoke_remote("stream.serialize", cyng::generate_invoke_reflect("db.trx.start"))
			, cyng::generate_invoke_remote("stream.flush")

			, cyng::generate_invoke_remote("bus.req.subscribe", table, cyng::code::IDENT, tsk)

			//	bounce back 
			, cyng::generate_invoke_remote("stream.serialize", cyng::generate_invoke_reflect("db.trx.commit"))
			, cyng::generate_invoke_remote("stream.flush")
			)

			<< cyng::generate_invoke("stream.flush")
			<< cyng::unwind_vec(72)	//	optimization
			;
	}

	cyng::vector_t bus_req_unsubscribe(std::string const& table)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"
			, cyng::generate_invoke_remote("bus.req.unsubscribe", table, cyng::code::IDENT))
			<< cyng::generate_invoke("stream.flush")
			<< cyng::unwind_vec()
			;
	}

	cyng::vector_t bus_res_subscribe(std::string const& table
		, cyng::vector_t const& key
		, cyng::vector_t const& data
		, std::uint64_t generation
		, boost::uuids::uuid tag
		, std::size_t tsk)
	{
		//	
		//	key and data will not be unwinded
		//
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.res.subscribe", table, key, data, generation, tag, tsk))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_req_db_insert(std::string const& table
		, cyng::vector_t const& key
		, cyng::vector_t const& data
		, std::uint64_t generation
		, boost::uuids::uuid source)
	{
		//	
		//	key and data will not be unwinded
		//
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("db.req.insert", table, key, data, generation, source))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_res_db_insert(std::string const& table
		, cyng::vector_t const& key
		, cyng::vector_t const& data
		, std::uint64_t generation)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("db.res.insert", table, key, data, generation))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_req_db_merge(std::string const& table
		, cyng::vector_t const& key
		, cyng::vector_t const& data
		, std::uint64_t generation
		, boost::uuids::uuid source)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("db.req.insert", table, key, data, generation, source))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_req_db_update(std::string const& table
		, cyng::vector_t const& key
		, cyng::vector_t const& data
		, std::uint64_t generation
		, boost::uuids::uuid source)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("db.req.update", table, key, data, generation, source))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_req_db_modify(std::string const& table
		, cyng::vector_t const& key
		, cyng::attr_t const& value
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("db.req.modify.by.attr", table, key, value, gen, source))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_req_db_modify(std::string const& table
		, cyng::vector_t const& key
		, cyng::param_t const& value
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("db.req.modify.by.param", table, key, value, gen, source))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_req_db_remove(std::string const& table
		, cyng::vector_t const& key
		, boost::uuids::uuid source)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("db.req.remove", table, key, source))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_res_db_remove(std::string const& table
		, cyng::vector_t const& key)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("db.res.remove", table, key))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_db_clear(std::string const& table)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("db.clear", table))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_req_watchdog()
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.req.watchdog", cyng::code::IDENT, cyng::invoke("bus.seq.next"), std::chrono::system_clock::now()))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_res_watchdog(std::uint64_t seq, std::chrono::system_clock::time_point tp)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.res.watchdog", cyng::code::IDENT, seq, tp))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_insert_msg(cyng::logging::severity level, std::string const& msg)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.insert.msg", cyng::code::IDENT, level, msg))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_insert_LoRa_uplink(cyng::object tp
		, cyng::mac64 devEUI
		, std::uint16_t FPort
		, std::uint32_t FCntUp
		, std::uint32_t ADRbit
		, std::uint32_t MType
		, std::uint32_t FCntDn
		, std::string const& customerID
		, std::string const& payload
		, boost::uuids::uuid tag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.insert.LoRa.uplink", cyng::code::IDENT, tp, devEUI, FPort, FCntUp, ADRbit, MType, FCntDn, customerID, payload, tag))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}


	cyng::vector_t bus_req_stop_client(cyng::vector_t const& key
		, boost::uuids::uuid source)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.req.stop.client", cyng::invoke("bus.seq.next"), key, source))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_req_com_sml(cyng::vector_t const& key
		, boost::uuids::uuid tag_ws
		, std::string channel
		, cyng::vector_t sections
		, cyng::vector_t params)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.req.gateway.proxy", cyng::code::IDENT, cyng::invoke("bus.seq.next"), key, tag_ws, channel, sections, params))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_res_com_sml(boost::uuids::uuid ident
		, boost::uuids::uuid source
		, std::uint64_t seq
		, cyng::vector_t key
		, boost::uuids::uuid tag_ws
		, std::string channel
		, std::string srv
		, std::string code
		, cyng::param_map_t const& params)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.res.gateway.proxy", ident, source, seq, key, tag_ws, channel, srv, code, params))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_res_attention_code(boost::uuids::uuid ident
		, boost::uuids::uuid source
		, std::uint64_t seq
		, boost::uuids::uuid tag_ws
		, std::string srv
		, cyng::buffer_t code
		, std::string msg)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.res.attention.code", ident, source, seq, tag_ws, srv, code, msg))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_req_com_task(boost::uuids::uuid key
		, boost::uuids::uuid tag_ws
		, std::string channel
		, cyng::vector_t sections
		, cyng::vector_t params)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.req.com.task", cyng::code::IDENT, cyng::invoke("bus.seq.next"), key, tag_ws, channel, sections, params))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_req_com_node(boost::uuids::uuid key
		, boost::uuids::uuid tag_ws
		, std::string channel
		, cyng::vector_t sections
		, cyng::vector_t params)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.req.com.node", cyng::code::IDENT, cyng::invoke("bus.seq.next"), key, tag_ws, channel, sections, params))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_req_modify_gateway(std::string section
		, cyng::vector_t const& key
		, boost::uuids::uuid source
		, cyng::param_map_t	params)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.req.modify.gateway", cyng::invoke("bus.seq.next"), section, key, source, params))
			//<< cyng::generate_invoke_unwinded("store.relation", cyng::invoke("bus.seq.push"), tag_ws)	//	store seq => ws relation
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_req_push_data(std::string const& class_name
		, std::string const& channel_name
		, bool distribution	//	single, all
		, cyng::vector_t const& key
		, cyng::vector_t const& data
		, boost::uuids::uuid source)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.req.push.data", cyng::invoke("bus.seq.next"), class_name, channel_name, distribution, key, data, source))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_req_push_data(std::uint64_t seq
		, std::string const& channel_name
		, cyng::vector_t const& key 
		, cyng::vector_t const& data
		, boost::uuids::uuid source)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.req.push.data", seq, channel_name, key, data, source))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_res_push_data(std::uint64_t seq
		, std::string const& class_name
		, std::string const& channel_name
		, std::size_t count)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("bus.res.push.data", seq, class_name, channel_name, count))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;

	}

	cyng::vector_t bus_res_db_modify(std::string const& table
		, cyng::vector_t const& key
		, cyng::attr_t const& attr
		, std::uint64_t gen)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("db.res.modify.by.attr", table, key, attr, gen))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t bus_res_db_modify(std::string const& table
		, cyng::vector_t const& key
		, cyng::param_t&& param)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("db.res.modify.by.param", table, key, param))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_req_login(boost::uuids::uuid tag
		, std::string const& name
		, std::string const& pwd
		, std::string const& scheme //	login scheme
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.req.login"
				, tag
				, cyng::code::IDENT
				, cyng::invoke("bus.seq.next")
				, name
				, pwd
				, scheme
				, bag
				, cyng::invoke_remote("push.session")))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_res_login(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, std::string const& name
		, std::string const& msg
		, std::uint32_t query
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"
			, cyng::generate_invoke_remote("client.res.login", tag, cyng::code::IDENT, seq, success, name, msg, query, bag))
			<< cyng::generate_invoke("stream.flush")
			<< cyng::unwind_vec()
			;
	}

	cyng::vector_t client_req_close(boost::uuids::uuid tag, int value)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.req.close", tag, cyng::code::IDENT, cyng::invoke("bus.seq.next"), value))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_res_close(boost::uuids::uuid tag, std::uint64_t seq, bool success)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.res.close", tag, cyng::code::IDENT, seq, success))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_req_gateway_proxy(boost::uuids::uuid tag
		, boost::uuids::uuid source
		, std::uint64_t seq
		, cyng::vector_t key
		, boost::uuids::uuid ws
		, std::string const& channel
		, cyng::vector_t sections
		, cyng::vector_t params
		, cyng::buffer_t const& server
		, std::string const& user
		, std::string const& pwd)
	{
			cyng::vector_t prg;
			return prg << cyng::generate_invoke_unwinded("stream.serialize"
				, cyng::generate_invoke_remote_unwinded("client.req.gateway.proxy", tag, cyng::code::IDENT, source, seq, key, ws, channel, sections, params, server, user, pwd))
				<< cyng::generate_invoke_unwinded("stream.flush")
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
			, cyng::generate_invoke_remote("client.req.open.push.channel", tag, cyng::code::IDENT, cyng::invoke("bus.seq.next"), target, device, number, version, id, std::chrono::seconds(timeout), bag))
			<< cyng::generate_invoke("stream.flush")
			<< cyng::unwind_vec()
			;
	}

	cyng::vector_t  client_req_close_push_channel(boost::uuids::uuid tag
		, std::uint32_t channel
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"
			, cyng::generate_invoke_remote("client.req.close.push.channel", tag, cyng::code::IDENT, cyng::invoke("bus.seq.next"), channel, bag))
			<< cyng::generate_invoke("stream.flush")
			<< cyng::unwind_vec()
			;
	}

	cyng::vector_t client_res_open_push_channel(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, std::uint32_t channel
		, std::uint32_t source
		, std::uint32_t count
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"
			, cyng::generate_invoke_remote("client.res.open.push.channel"
				, tag
				, cyng::code::IDENT
				, seq
				, success
				, channel
				, source
				, count
				, options
				, bag))
			<< cyng::generate_invoke("stream.flush")
			<< cyng::unwind_vec()
			;
	}

	cyng::vector_t client_res_close_push_channel(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, std::uint32_t channel
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.res.close.push.channel"
				, tag
				, cyng::code::IDENT
				, seq
				, success
				, channel
				, bag))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_req_register_push_target(boost::uuids::uuid tag
		, std::string const& target
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"
			, cyng::generate_invoke_remote("client.req.register.push.target"
				, tag
				, cyng::code::IDENT
				, cyng::invoke("bus.seq.next")
				, target
				, bag))
			<< cyng::generate_invoke("stream.flush")
			<< cyng::unwind_vec()
			;
	}

	cyng::vector_t client_req_deregister_push_target(boost::uuids::uuid tag
		, std::string const& target
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"
			, cyng::generate_invoke_remote("client.req.deregister.push.target"
				, tag
				, cyng::code::IDENT
				, cyng::invoke("bus.seq.next")
				, target
				, bag))
			<< cyng::generate_invoke("stream.flush")
			<< cyng::unwind_vec()
			;
	}

	cyng::vector_t client_res_register_push_target(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, std::uint32_t channel
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"
			, cyng::generate_invoke_remote("client.res.register.push.target"
				, tag
				, cyng::code::IDENT
				, seq
				, success
				, channel
				, options
				, bag))
			<< cyng::generate_invoke("stream.flush")
			<< cyng::unwind_vec()
			;
	}

	cyng::vector_t client_res_deregister_push_target(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, std::string const& target
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke("stream.serialize"
			, cyng::generate_invoke_remote("client.res.deregister.push.target"
				, tag
				, cyng::code::IDENT
				, seq
				, success
				, target
				, options
				, bag))
			<< cyng::generate_invoke("stream.flush")
			<< cyng::unwind_vec()
			;
	}

	cyng::vector_t client_req_open_connection(boost::uuids::uuid tag
		, std::string const& number
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.req.open.connection"
				, tag
				, cyng::code::IDENT
				, cyng::invoke("bus.seq.next")
				, number
				, bag
				, cyng::invoke_remote("push.session")))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_req_open_connection_forward(boost::uuids::uuid tag
		, std::string const& number
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.req.open.connection.forward"
				, tag
				, cyng::code::IDENT
				, cyng::invoke("bus.seq.next")
				, number
				, options
				, bag))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_res_open_connection_forward(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.res.open.connection.forward"
				, tag
				, cyng::code::IDENT
				, seq
				, success
				, options
				, bag))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_res_close_connection_forward(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.res.close.connection.forward"
				, tag
				, cyng::code::IDENT
				, seq
				, success
				, options
				, bag))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;

	}

	cyng::vector_t client_res_open_connection(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.res.open.connection"
				, tag
				, cyng::code::IDENT
				, seq
				, success
				, options
				, bag))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_req_close_connection(boost::uuids::uuid rtag
		, bool shutdown
		, cyng::param_map_t&& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.req.close.connection"
				, rtag
				, cyng::code::IDENT
				, shutdown
				, cyng::invoke("bus.seq.next")
				, bag))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_req_close_connection_forward(boost::uuids::uuid rtag
		, boost::uuids::uuid origin_tag
		, std::uint64_t seq
		, bool shutdown
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.req.close.connection.forward"
				, rtag
				, cyng::code::IDENT
				, seq
				, origin_tag
				, shutdown
				, options
				, bag))
			//<< cyng::generate_invoke_unwinded("bus.store.req.connection.close", seq, rtag, bag)
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_res_close_connection(boost::uuids::uuid tag
		, std::uint64_t seq
		, bool success
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.res.close.connection"
				, tag
				, cyng::code::IDENT
				, seq
				, success
				, options
				, bag))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;

	}

	cyng::vector_t client_req_transmit_data(boost::uuids::uuid tag
		, cyng::param_map_t bag
		, cyng::object const& data)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.req.transmit.data"
				, tag
				, cyng::code::IDENT
				, cyng::invoke("bus.seq.next")
				, bag
				, data))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_req_transfer_data_forward(boost::uuids::uuid tag
		, std::uint64_t seq
		, cyng::param_map_t bag
		, cyng::object const& data)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.req.transmit.data.forward"
				, tag
				, cyng::code::IDENT
				, seq
				, bag
				, data))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_req_transfer_pushdata(boost::uuids::uuid tag
		, std::uint32_t source
		, std::uint32_t channel
		, cyng::object data
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.req.transfer.pushdata"
				, tag
				, cyng::code::IDENT
				, cyng::invoke("bus.seq.next")
				, source 
				, channel
				, bag
				, data))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_req_transfer_pushdata_forward(boost::uuids::uuid tag
		, std::uint32_t channel
		, std::uint32_t source
		, std::uint32_t target
		, cyng::object data
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.req.transfer.pushdata.forward"
				, tag
				, cyng::code::IDENT
				, cyng::invoke("bus.seq.next")
				, channel
				, source
				, target
				, bag
				, data))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_res_transfer_pushdata(boost::uuids::uuid tag
		, std::uint64_t seq
		, std::uint32_t source
		, std::uint32_t channel
		, std::size_t count		//	count
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.res.transfer.pushdata"
				, tag
				, cyng::code::IDENT
				, seq
				, source
				, channel
				, count
				, bag))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_update_attr(boost::uuids::uuid tag
		, std::string attr
		, cyng::object value
		, cyng::param_map_t const& bag)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.update.attr"
				, tag
				, cyng::code::IDENT
				, cyng::invoke("bus.seq.next")
				, attr
				, bag
				, value))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;
	}

	cyng::vector_t client_inc_throughput(boost::uuids::uuid origin
		, boost::uuids::uuid target
		, std::uint64_t size)
	{
		cyng::vector_t prg;
		return prg << cyng::generate_invoke_unwinded("stream.serialize"
			, cyng::generate_invoke_remote_unwinded("client.inc.throughput"
				, origin
				, target
				, cyng::code::IDENT
				, size))
			<< cyng::generate_invoke_unwinded("stream.flush")
			;

	}

}
