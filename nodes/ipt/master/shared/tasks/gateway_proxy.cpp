/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "gateway_proxy.h"

#include <smf/sml/srv_id_io.h>
#include <smf/sml/protocol/generator.h>
#include <smf/cluster/generator.h>
#include <smf/sml/status.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/parser/srv_id_parser.h>
#include <smf/sml/parser/obis_parser.h>
#include <smf/sml/event.h>
#include <smf/sml/intrinsics/obis_factory.hpp>

#include <cyng/vm/generator.h>
#include <cyng/sys/mac.h>
#include <cyng/io/io_bytes.hpp>
#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/vector_cast.hpp>
#include <cyng/parser/buffer_parser.h>
#include <cyng/async/task/task_builder.hpp>

#ifdef SMF_IO_LOG
#include <cyng/io/hex_dump.hpp>
#include <boost/uuid/uuid_io.hpp>
#endif

#ifdef _DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

namespace node
{
	gateway_proxy::gateway_proxy(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, bus::shared_type bus
		, cyng::controller& vm
		, std::chrono::seconds timeout
		, bool sml_log)
	: base_(*btp)
		, logger_(logger)
		, bus_(bus)
		, vm_(vm)
		, timeout_(timeout)
		, start_(std::chrono::system_clock::now())
		, parser_([this](cyng::vector_t&& prg) {

			//CYNG_LOG_DEBUG(logger_, "sml processor - "
			//	<< prg.size()
			//	<< " instructions");

			//CYNG_LOG_TRACE(logger_, cyng::io::to_str(prg));

			//
			//	execute SML programm
			//
			vm_.async_run(std::move(prg));

		}, false, sml_log, false)	//	not verbose, no log instructions
		, input_queue_()
		, output_map_()
		, open_requests_{ 0 }
		, state_{ GWPS::OFFLINE_ }
		, config_cache_()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is running with "
			<< cyng::to_str(timeout_)
			<< " interval");
	}

	cyng::continuation gateway_proxy::run()
	{	
		switch (state_) {
		case GWPS::OFFLINE_:
		case GWPS::WAITING_:
			//
			//	waiting for an opportunity to open a connection. If session is ready
			//	is signals OK on slot 0
			//
			if (!input_queue_.empty()) {

				//
				//	update state
				//
				state_ = GWPS::WAITING_;

				//
				//	send connect request
				//
				vm_.async_run(cyng::generate_invoke("session.redirect", base_.get_id()));
			}
			break;
		case GWPS::CONNECTED_:
			if (!input_queue_.empty()) {
				//
				//	run input queue
				//
				run_queue();
			}
			break;
		default:
			break;
		}

		//
		//	re-start monitor
		//
		base_.suspend(timeout_);

		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot 0 - ack (connected to gateway)
	cyng::continuation gateway_proxy::process()
	{
		BOOST_ASSERT_MSG(state_ == GWPS::WAITING_, "wrong state");
		if (!input_queue_.empty()) {

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> gain access and processing input queue with "
				<< input_queue_.size()
				<< " entries");

			//
			//	update state
			//
			state_ = GWPS::CONNECTED_;

			//
			//	clear remaining entries
			//
			output_map_.clear();

			//
			//	run input queue and process commands
			//
			run_queue();

		}
		else {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> got connected but queue is empty - disconnect");


			//
			//	reset connection state to NOT-CONNECTED == authorized
			//
			vm_.async_run(cyng::generate_invoke("session.redirect", static_cast<std::size_t>(cyng::async::NO_TASK)));

			//
			//	update state
			//
			state_ = GWPS::OFFLINE_;
		}

		//
		//	read SML data
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot 1 - EOM
	cyng::continuation gateway_proxy::process(std::uint16_t crc, std::size_t midx)
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> EOM #"
			<< midx);


		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot 2 - receive data
	cyng::continuation gateway_proxy::process(cyng::buffer_t const& data)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> received "
			<< cyng::bytes_to_str(data.size()));

#ifdef _DEBUG
		std::stringstream ss;
		ss << "SML response:" << std::endl;
		cyng::io::hex_dump hd;
		hd(ss, data.begin(), data.end());
		CYNG_LOG_DEBUG(logger_, ss.str());
#endif

		//
		//	parse SML data stream
		//
		parser_.read(data.begin(), data.end());

		return cyng::continuation::TASK_CONTINUE;
	}

	//	-- slot[3]: GetProcParam.Res
	cyng::continuation gateway_proxy::process(std::string trx
		, std::uint8_t group
		, cyng::buffer_t srv_id
		, cyng::vector_t vec
		, cyng::param_t values)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< sml::messages::name(sml::message_e::GET_PROC_PARAMETER_RESPONSE)
			<< " #"
			<< trx);

		auto const path = sml::vector_to_path(vec);

		auto const pos = output_map_.find(trx);
		if (pos != output_map_.end()) {

			//
			//	get root path
			//
			sml::obis const root(path.front());

			CYNG_LOG_DEBUG(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< sml::messages::name(sml::message_e::GET_PROC_PARAMETER_RESPONSE)
				<< " #"
				<< trx
				<< " - "
				<< sml::get_name(root)
				<< " - "
				<< cyng::io::to_str(pos->second.get_params()));

			//
			//	get server ID as string
			//
			auto const srv_str = (srv_id.empty())
				? sml::from_server_id(pos->second.get_srv())
				: sml::from_server_id(srv_id)
				;

			//
			//	extract the relevant data 
			//
			auto params = cyng::to_param_map(values.second);

			//
			//	update configuration cache
			//
			update_cfg_cache(srv_str, path, pos->second, params, pos->second.is_job());


			if (pos->second.is_job()) {

				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> job result for trx "
					<< trx
					<< " - "
					<< sml::to_hex(path, ':')
					<< " from "
					<< srv_str
					<< "/"
					<< sml::from_server_id(pos->second.get_srv())
					<< " ["
					<< sml::messages::name(pos->second.get_msg_code())
					<< "]: "
					<< cyng::io::to_str(params));

				process_job_result(path, pos->second, params);

			}
			else {

				if (sml::OBIS_CLASS_OP_LOG_STATUS_WORD == root) {

					bus_->vm_.async_run(bus_res_com_sml(pos->second.get_tag_ident()
						, pos->second.get_tag_source()
						, pos->second.get_sequence()
						, pos->second.get_key_gw()
						, pos->second.get_tag_origin()
						, sml::messages::name(sml::message_e::GET_PROC_PARAMETER_RESPONSE)
						, srv_str
						, transform_to_str_vector(path, false)	//	vector of string
						, transform_status_word(values.second)));

				}
				else if (sml::OBIS_ROOT_BROKER == root) {

					CYNG_LOG_DEBUG(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> OBIS_ROOT_BROKER: "
						<< cyng::io::to_type(params));

					bus_->vm_.async_run(bus_res_com_sml(pos->second.get_tag_ident()
						, pos->second.get_tag_source()
						, pos->second.get_sequence()
						, pos->second.get_key_gw()
						, pos->second.get_tag_origin()
						, sml::messages::name(sml::message_e::GET_PROC_PARAMETER_RESPONSE)
						, srv_str
						, transform_to_str_vector(path, false)	//	vector of string
						, transform_broker_params(logger_, params)));

				}
				else if (sml::OBIS_ROOT_HARDWARE_PORT == root) {

					CYNG_LOG_DEBUG(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> ROOT_HARDWARE_PORT: "
						<< cyng::io::to_type(params));

					bus_->vm_.async_run(bus_res_com_sml(pos->second.get_tag_ident()
						, pos->second.get_tag_source()
						, pos->second.get_sequence()
						, pos->second.get_key_gw()
						, pos->second.get_tag_origin()
						, sml::messages::name(sml::message_e::GET_PROC_PARAMETER_RESPONSE)
						, srv_str
						, transform_to_str_vector(path, false)	//	vector of string
						, transform_broker_hw_params(logger_, params)));

				}
				else if (sml::OBIS_ROOT_DATA_COLLECTOR == root) {

					CYNG_LOG_DEBUG(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> ROOT_DATA_COLLECTOR: "
						<< cyng::io::to_type(params));

					bus_->vm_.async_run(bus_res_com_sml(pos->second.get_tag_ident()
						, pos->second.get_tag_source()
						, pos->second.get_sequence()
						, pos->second.get_key_gw()
						, pos->second.get_tag_origin()
						, sml::messages::name(sml::message_e::GET_PROC_PARAMETER_RESPONSE)
						, srv_str
						, transform_to_str_vector(path, false)	//	vector of string
						, transform_data_collector_params(logger_, params)));
				}
				else {

					CYNG_LOG_TRACE(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> response "
						<< trx
						<< " - "
						<< sml::to_hex(path, ':')
						<< " from "
						<< srv_str
						<< "/"
						<< sml::from_server_id(pos->second.get_srv())
						<< " ["
						<< sml::messages::name(sml::message_e::GET_PROC_PARAMETER_RESPONSE)
						<< "]: "
						<< cyng::io::to_str(params));

					bus_->vm_.async_run(bus_res_com_sml(pos->second.get_tag_ident()
						, pos->second.get_tag_source()
						, pos->second.get_sequence()
						, pos->second.get_key_gw()
						, pos->second.get_tag_origin()
						, sml::messages::name(sml::message_e::GET_PROC_PARAMETER_RESPONSE)
						, srv_str
						, transform_to_str_vector(path, false)	//	vector of string
						, params));
				}
			}

			//
			//	remove from map
			//
			output_map_.erase(pos);

		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> response "
				<< trx
				<< " not found");
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	//	-- slot[4]
	cyng::continuation gateway_proxy::process(std::string trx)
	{
		//
		//	current SML request is complete
		//	sml_public_close_response
		//
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> sml.public.close.response ["
			<< trx
			<< "] open requests: "
			<< open_requests_
			<< " , input queue size: "
			<< input_queue_.size()
			<< ", output map: "
			<< output_map_.size());

		//
		//	update request counter
		//
		--open_requests_;

		if ((open_requests_ == 0u) && input_queue_.empty()) {

			//BOOST_ASSERT_MSG(input_queue_.empty(), "input queue is not empty");
			//
			//	update task state
			//
			state_ = GWPS::OFFLINE_;

			//
			//	terminate redirection
			//
			vm_.async_run(cyng::generate_invoke("session.redirect", static_cast<std::size_t>(cyng::async::NO_TASK)));

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> cancel redirection");
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	//	-- slot[5]: GetList.Res
	cyng::continuation gateway_proxy::process(std::string trx
		, cyng::buffer_t srv_id
		, cyng::vector_t vec	//	OBIS (path)
		, cyng::param_map_t params)
	{
		auto const path = sml::vector_to_path(vec);

		//
		//	There should be entries
		//
		auto const pos = output_map_.find(trx);
		if (pos != output_map_.end()) {

			//
			//	get root path
			//
			sml::obis const root(path.front());

			//
			//	get server ID as string
			//
			auto const srv_str = (srv_id.empty())
				? sml::from_server_id(pos->second.get_srv())
				: sml::from_server_id(srv_id)
				;


			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> response "
				<< trx
				<< " - "
				<< sml::to_hex(path, ':')
				<< " from "
				<< srv_str
				<< "/"
				<< sml::from_server_id(pos->second.get_srv())
				<< " ["
				<< sml::messages::name(pos->second.get_msg_code())
				<< "]: "
				<< cyng::io::to_str(params));

			//
			//	send response to requester
			//
			bus_->vm_.async_run(bus_res_com_sml(pos->second.get_tag_ident()
				, pos->second.get_tag_source()
				, pos->second.get_sequence()
				, pos->second.get_key_gw()
				, pos->second.get_tag_origin()
				//	substitute request with response name
				, sml::messages::name(sml::messages::get_response(pos->second.get_msg_code()))
				, srv_str
				, transform_to_str_vector(path, false)	//	vector of string
				, params));

			//
			//	update configuration cache
			//
			update_cfg_cache(srv_str, path, pos->second, params, false);

			//
			//	remove from map
			//
			output_map_.erase(pos);

		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> response "
				<< trx
				<< " not found");
		}
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation gateway_proxy::process(std::string trx
		, std::string srv
		, cyng::buffer_t const& code)
	{

		auto const pos = output_map_.find(trx);
		if (pos != output_map_.end()) {

			sml::obis attention(code);

			if (!pos->second.is_job()) {

				CYNG_LOG_INFO(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> response #"
					<< pos->second.get_sequence()
					<< " from "
					<< srv
					<< " attention code "
					<< sml::get_attention_name(attention));

				bus_->vm_.async_run(bus_res_attention_code(pos->second.get_tag_ident()
					, pos->second.get_tag_source()
					, pos->second.get_sequence()
					, pos->second.get_tag_origin()
					, srv
					, code
					, sml::get_attention_name(attention)));
			}

			//
			//	remove entry
			//
			output_map_.erase(pos);
		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> response "
				<< trx
				<< " not found");
		}
		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot [8]: GetProfileList.Res
	cyng::continuation gateway_proxy::process(std::string trx
		, std::uint32_t act_time
		, std::uint32_t reg_period
		, std::uint32_t val_time
		, cyng::vector_t vec
		, cyng::buffer_t srv_id
		, std::uint32_t	stat
		, cyng::param_map_t params)
	{
		auto const path = sml::vector_to_path(vec);

		auto const pos = output_map_.find(trx);
		if (pos != output_map_.end()) {

			//
			//	get root path
			//
			sml::obis const root(path.front());

			//
			//	get server ID as string
			//
			auto const srv_str = (srv_id.empty())
				? sml::from_server_id(pos->second.get_srv())
				: sml::from_server_id(srv_id)
				;

			//
			//	update params with specific SML_GetProfileList.Res data
			//
			params.emplace("actTime", cyng::make_object(act_time));
			params.emplace("regPeriod", cyng::make_object(reg_period));
			params.emplace("valTime", cyng::make_object(val_time));
			params.emplace("status", cyng::make_object(stat));

			//
			//	provide more event type data
			//
#ifdef __DEBUG
			auto const evt = params.find(sml::OBIS_CLASS_EVENT.to_str());
			if (evt != params.end()) {
				auto const val = cyng::value_cast<std::uint32_t>(evt->second, 0u);
				params.emplace("evtType", cyng::make_object(sml::evt_get_type(val)));
				params.emplace("evtLevel", cyng::make_object(sml::evt_get_level(val)));
				params.emplace("evtSource", cyng::make_object(sml::evt_get_source(val)));
			}
#endif

			//	[2159338-2,084e0f4a,0384,084dafce,<!18446744073709551615:user-defined>,0500153B021774,00070202,%(("010000090B00":{7,0,2019-10-10 15:23:50.00000000,null}),("81040D060000":{ff,0,0,null}),("810417070000":{ff,0,0,null}),("81041A070000":{ff,0,0,null}),("81042B070000":{fe,0,0,null}),("8181000000FF":{ff,0,818100000001,null}),("8181C789E2FF":{ff,0,00800000,null}))]]
			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> response - "
				<< sml::to_hex(path, ':')
				<< " #"
				<< pos->second.get_sequence()
				<< " from "
				<< srv_str
				<< " status "
				<< stat);

			bus_->vm_.async_run(bus_res_com_sml(pos->second.get_tag_ident()
				, pos->second.get_tag_source()
				, pos->second.get_sequence()
				, pos->second.get_key_gw()
				, pos->second.get_tag_origin()
				//	replace request with response name
				, sml::messages::name(sml::messages::get_response(pos->second.get_msg_code()))
				, srv_str
				, transform_to_str_vector(path, false)	//	vector of string
				, params));

			//
			//	update configuration cache
			//
			update_cfg_cache(srv_str, path, pos->second, params, false);

		}
		else {
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> response "
				<< trx
				<< " not found");
		}
		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot [7]
	cyng::continuation gateway_proxy::process(
		boost::uuids::uuid tag,		//	[0] ident tag (target)
		boost::uuids::uuid source,	//	[1] source tag
		std::uint64_t seq,			//	[2] cluster seq
		boost::uuids::uuid origin,	//	[3] ws tag (origin)

		std::string channel,		//	[4] channel (SML message type)
		cyng::buffer_t code,		//	[5] OBIS root code
		cyng::vector_t gw,			//	[6] TGateway/TDevice PK
		cyng::tuple_t params,		//	[7] parameters

		cyng::buffer_t srv_id,		//	[8] server id
		std::string name,			//	[9] name
		std::string	pwd,			//	[10] pwd
		bool cache_enabled			//	[11] use cache
	)
	{

		//
		//	get root path
		//
		sml::obis const root(code);

		//
		//	make some adjustments
		//
		auto pd = finalize(proxy_data(tag
			, source
			, seq
			, origin
			, channel
			, sml::obis_path_t({ root })
			, gw
			, params
			, srv_id
			, name
			, pwd
			, false));	//	no job

		if (cache_enabled 
			&& config_cache_.is_cached(pd.get_path())
			&& sml::message_e::GET_PROC_PARAMETER_REQUEST == pd.get_msg_code()) {

			//
			//	get data from cache
			//

			if (sml::OBIS_ROOT_BROKER == root) {

				bus_->vm_.async_run(bus_res_com_sml(tag
					, source
					, seq
					, gw
					, origin
					, sml::messages::name(sml::message_e::GET_PROC_PARAMETER_RESPONSE)
					, sml::from_server_id(srv_id)
					, transform_to_str_vector(pd.get_path(), false)	//	vector of string
					, transform_broker_params(logger_, config_cache_.get_section(pd.get_path()))));


			}
			else if (sml::OBIS_ROOT_HARDWARE_PORT == root) {

				CYNG_LOG_DEBUG(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> ROOT_HARDWARE_PORT: "
					<< cyng::io::to_type(params));

				bus_->vm_.async_run(bus_res_com_sml(tag
					, source
					, seq
					, gw
					, origin
					, sml::messages::name(sml::message_e::GET_PROC_PARAMETER_RESPONSE)
					, sml::from_server_id(srv_id)
					, transform_to_str_vector(pd.get_path(), false)	//	vector of string
					, transform_broker_hw_params(logger_, config_cache_.get_section(pd.get_path()))));

			}
			else if (sml::OBIS_ROOT_DATA_COLLECTOR == root) {

				CYNG_LOG_DEBUG(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> ROOT_DATA_COLLECTOR: "
					<< cyng::io::to_type(params));

				bus_->vm_.async_run(bus_res_com_sml(tag
					, source
					, seq
					, gw
					, origin
					, sml::messages::name(sml::message_e::GET_PROC_PARAMETER_RESPONSE)
					, sml::from_server_id(srv_id)
					, transform_to_str_vector(pd.get_path(), false)	//	vector of string
					, transform_data_collector_params(logger_, config_cache_.get_section(pd.get_path()))));
			}
			else {
				bus_->vm_.async_run(bus_res_com_sml(tag
					, source
					, seq
					, gw
					, origin
					, sml::messages::name(sml::message_e::GET_PROC_PARAMETER_RESPONSE)
					, sml::from_server_id(srv_id)
					, transform_to_str_vector(pd.get_path(), false)	//	vector of string
					, config_cache_.get_section(pd.get_path())));
			}
		}
		else {

			//
			//	add to active cache sections id global caching flag is set
			//
			if (cache_enabled) {
				config_cache_.add(srv_id, pd.get_path());
			}

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> new input queue entry "
				<< sml::to_hex(pd.get_path(), ':')
				<< sml::get_name(root)
				<< " - size: "
				<< input_queue_.size());

			//
			//	get data from gateway
			//
			input_queue_.emplace(std::move(pd));

			//
			//	preprocessing
			//


			BOOST_ASSERT(!input_queue_.empty());

			switch (state_) {
			case GWPS::OFFLINE_:

				//
				//	update state
				//
				state_ = GWPS::WAITING_;

				//
				//	waiting for an opportunity to open a connection. If session is ready
				//	is signals OK on slot [0]
				vm_.async_run(cyng::generate_invoke("session.redirect", base_.get_id()));

				break;
			default:
				break;
			}
		}
		return cyng::continuation::TASK_CONTINUE;
	}

	//	slot [9]
	cyng::continuation gateway_proxy::process(boost::uuids::uuid tag,
		boost::uuids::uuid source,
		std::uint64_t seq,			
		boost::uuids::uuid origin,
		std::string job,			
		cyng::vector_t pk,			
		cyng::vector_t sections,	
		cyng::buffer_t srv_id,		
		std::string name,			
		std::string pwd)			
	{
		//
		//	expecting obis codes
		//
		auto secs = cyng::vector_cast<std::string>(sections, "");

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< sml::from_server_id(srv_id)
			<< " executes job: "
			<< job
			<< " with "
			<< secs.size()
			<< " section(s)");

		if (boost::algorithm::equals(job, "cache.reset")) {

			/*
			 * Make configuration cache ready to store the specfied paths.
			 */
			config_cache_.reset(srv_id, sml::vector_to_path(secs));

			bus_->vm_.async_run(bus_res_com_proxy(tag
				, source
				, seq
				, pk
				, origin
				, job
				, sml::from_server_id(srv_id)
				, sections
				, cyng::param_map_factory()));

		}
		else if (boost::algorithm::equals(job, "cache.sections")) {

			//
			//	get all root sections
			//
			auto const roots = config_cache_.get_root_sections();

			bus_->vm_.async_run(bus_res_com_proxy(tag
				, source
				, seq
				, pk
				, origin
				, job
				, sml::from_server_id(srv_id)
				, sml::path_to_vector(roots)
				, cyng::param_map_factory()));
		}
		else if (boost::algorithm::equals(job, "cache.update")) {

			//
			//	get root paths to update
			//
			auto const roots = sml::vector_to_path(secs);
			for (auto const& code : roots) {

				switch (code.to_uint64()) {
				case sml::CODE_ROOT_ACCESS_RIGHTS:
					start_job_access_rights(tag, source, seq, origin, pk, srv_id, name, pwd);
					break;
				default:
					//
					//	using the same input queue as for other requests
					//
					input_queue_.emplace(tag
						, source
						, seq
						, origin
						, "GetProcParameterRequest"
						, sml::obis_path_t({ code })
						, pk
						, cyng::tuple_t{}
						, srv_id
						, name
						, pwd
						, true);	//	job

					CYNG_LOG_TRACE(logger_, "task #"
						<< base_.get_id()
						<< " <"
						<< base_.get_class_name()
						<< "> "
						<< sml::from_server_id(srv_id)
						<< " update section "
						<< sml::get_name(code)
						<< " new input queue entry - size: "
						<< input_queue_.size());

					break;
				}
			}
		}
		else if (boost::algorithm::equals(job, "cache.sync")) {

			//
			//	synchronize configuration cache to master node
			//
			bus_->vm_.async_run(bus_res_com_proxy(tag
				, source
				, seq
				, pk
				, origin
				, job
				, sml::from_server_id(srv_id)
				, sections
				, cyng::param_map_factory("status", "not implemented yet")));
		}
		else if (boost::algorithm::equals(job, "cache.query")) {

			//
			//	send the content of the cache
			//
			//
			//	get root paths to update
			//
			auto const roots = sml::vector_to_path(secs);
			for (auto const& code : roots) {

				switch (code.to_uint64()) {
				case sml::CODE_ROOT_ACCESS_RIGHTS:
					send_access_rights_from_cache(tag, source, seq, origin, pk, srv_id, name, pwd);
					break;
				default:
					break;
				}
			}

		}
		else {
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> unknown job: "
				<< job);

		}
		return cyng::continuation::TASK_CONTINUE;
	}

	void gateway_proxy::run_queue()
	{
		//
		//	this loop is atomic
		//
		while (!input_queue_.empty()) {

			auto const server_id = input_queue_.front().get_srv();
			auto const user = input_queue_.front().get_user();
			auto const pwd = input_queue_.front().get_pwd();
			auto const source = input_queue_.front().get_tag_source();

			//
			//	generate public open request
			//
			node::sml::req_generator sml_gen(user, pwd);
			auto trx = sml_gen.public_open(get_mac(), server_id);
			boost::ignore_unused(trx);

			//	note that we take a reference
			auto const& data = input_queue_.front();
			auto const params = cyng::make_reader(data.get_params());

			//
			//	execute top queue entry
			//
			execute_cmd(sml_gen, data, params);

			//
			//	generate close request
			//
			trx = sml_gen.public_close();
			boost::ignore_unused(trx);
			cyng::buffer_t msg = sml_gen.boxing();


			//
			//	update data throughput (outgoing)
			//
			bus_->vm_.async_run(client_inc_throughput(vm_.tag()
				, source
				, msg.size()));

			//
			//	update request counter
			//
			++open_requests_;

#ifdef SMF_IO_LOG
			cyng::io::hex_dump hd;
			hd(std::cerr, msg.begin(), msg.end());
#else
			CYNG_LOG_DEBUG(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> #"
				<< open_requests_
				<< " send "
				<< cyng::bytes_to_str(msg.size())
				<< " to gateway: "
				<< cyng::io::to_hex(msg))
#endif

			//
			//	send to gateway
			//
			vm_.async_run({ cyng::generate_invoke("ipt.transfer.data", std::move(msg))
				, cyng::generate_invoke("stream.flush") });

			//
			//	remove top queue entry
			//
			input_queue_.pop();
		}

		BOOST_ASSERT(input_queue_.empty());
	}

	void gateway_proxy::execute_cmd(sml::req_generator& sml_gen
		, proxy_data const& data
		, cyng::tuple_reader const& params)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> execute channel "
			<< sml::messages::name(data.get_msg_code()));
		
		switch (data.get_msg_code()) {

		case sml::message_e::GET_PROFILE_PACK_REQUEST:
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> not implemented yet "
				<< sml::messages::name(data.get_msg_code()));
			break;

		case sml::message_e::GET_PROFILE_LIST_REQUEST:
			execute_cmd_get_profile_list(sml_gen, data, params);
			break;

		case sml::message_e::GET_PROC_PARAMETER_REQUEST:
			push_trx(sml_gen.get_proc_parameter(data.get_srv(), data.get_path()), data);
			break;

		case sml::message_e::SET_PROC_PARAMETER_REQUEST:
			execute_cmd_set_proc_param(sml_gen, data, params);
			break;

		case sml::message_e::GET_LIST_REQUEST:
			execute_cmd_get_list_request(sml_gen, data, params);
			break;

		default:
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> unknown channel "
				<< sml::messages::name(data.get_msg_code()));

			break;
		}
	}

	void gateway_proxy::execute_cmd_get_profile_list(sml::req_generator& sml_gen
		, proxy_data const& data
		, cyng::tuple_reader const& params)
	{
		if (sml::OBIS_CLASS_OP_LOG == data.get_root()) {

			//
			//	get time range
			//
			auto const hours = cyng::numeric_cast<std::uint32_t>(params.get("range"), 24u);

			//
			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< sml::messages::name(data.get_msg_code())
				<< " - range: "
				<< hours
				<< " hours");

			//	[
			//		cb4d6c74-be4c-481b-8317-7b0c03191ed6,
			//		9f773865-e4af-489a-8824-8f78a2311278,
			//		75,
			//		ad4d7f18-0968-4fd9-8904-dd9296cc8132,
			//		GetProfileListRequest,
			//		8181C789E1FF,
			//		[bd065994-fe7b-4985-b2ab-d9d64082ecfe],
			//		{("range":144)},
			//		0500153B02297E,
			//		operator,
			//		operator
			//	]

			push_trx(sml_gen.get_profile_list(data.get_srv()
				, std::chrono::system_clock::now() - std::chrono::hours(hours)
				, std::chrono::system_clock::now()
				, data.get_root()), data);	//	81 81 C7 89 E1 FF
		}
		else {

			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< sml::messages::name(data.get_msg_code())
				<< " - unknown path "
				<< data.get_root().to_str());
		}

	}


	void gateway_proxy::execute_cmd_set_proc_param(sml::req_generator& sml_gen
		, proxy_data const& data
		, cyng::tuple_reader const& params)
	{
		//
		//	generate set process parameter requests
		//

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> set "
			<< sml::from_server_id(data.get_srv())
			<< " root "
			<< data.get_root().to_str());

		switch (data.get_root().to_uint64()) {
		case sml::CODE_ROOT_IPT_PARAM:
			//
			//	modify IP-T parameters
			//
			execute_cmd_set_proc_param_ipt(sml_gen
				, data
				, cyng::numeric_cast<std::uint8_t>(params.get("index"), 0u)
				, cyng::to_tuple(params.get("ipt")));
			push_trx(sml_gen.get_proc_parameter(data.get_srv(), data.get_path()), data);
			break;

		case sml::CODE_IF_wMBUS:

			//
			//	modify wireless IEC parameters
			//
			execute_cmd_set_proc_param_wmbus(sml_gen, data, cyng::to_tuple(params.get("wmbus")));
			push_trx(sml_gen.get_proc_parameter(data.get_srv(), data.get_path()), data);
			break;

		case sml::CODE_IF_1107:
			//
			//	modify wired IEC parameters
			//
			execute_cmd_set_proc_param_iec(sml_gen, data, cyng::to_tuple(params.get("iec")));
			push_trx(sml_gen.get_proc_parameter(data.get_srv(), data.get_path()), data);
			break;

		case sml::CODE_REBOOT:
			//
			//	reboot the gateway.
			//	A "get process parameter request" doesn't make sense after reboot
			//
			sml_gen.set_proc_parameter_restart(data.get_srv());
			break;

		case sml::CODE_ACTIVATE_DEVICE:
			//
			//	activate meter device
			//
			if (execute_cmd_set_proc_param_activate(sml_gen
				, data
				, cyng::numeric_cast<std::uint8_t>(params.get("nr"), 0u)
				, cyng::value_cast<std::string>(params.get("meter"), ""))) {

				push_trx(sml_gen.get_proc_parameter(data.get_srv(), data.get_path()), data);
			}
			break;

		case sml::CODE_DEACTIVATE_DEVICE:
			//
			//	deactivate meter device
			//
			if (execute_cmd_set_proc_param_deactivate(sml_gen
				, data
				, cyng::numeric_cast<std::uint8_t>(params.get("nr"), 0u)
				, cyng::value_cast<std::string>(params.get("meter"), ""))) {

				push_trx(sml_gen.get_proc_parameter(data.get_srv(), data.get_path()), data);
			}
			break;

		case sml::CODE_DELETE_DEVICE:
			//
			//	remove a meter device from list of visible meters
			//
			if (execute_cmd_set_proc_param_delete(sml_gen
				, data, cyng::numeric_cast<std::uint8_t>(params.get("nr"), 0u)
				, cyng::value_cast<std::string>(params.get("meter"), ""))) {
				push_trx(sml_gen.get_proc_parameter(data.get_srv(), data.get_path()), data);
			}
			break;

		case sml::CODE_ROOT_SENSOR_PARAMS:

			CYNG_LOG_FATAL(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> OBIS_ROOT_SENSOR_PARAMS not implemented yet: "
				<< sml::OBIS_ROOT_SENSOR_PARAMS.to_str());

			if (execute_cmd_set_proc_param_meter(sml_gen
				, data
				, cyng::value_cast<std::string>(params.get("meter"), "")
				, cyng::value_cast<std::string>(params["data"].get("user"), "")
				, cyng::value_cast<std::string>(params["data"].get("pwd"), "")
				, cyng::value_cast<std::string>(params["data"].get("pubKey"), "")
				, cyng::value_cast<std::string>(params["data"].get("aesKey"), ""))) {

				push_trx(sml_gen.get_proc_parameter(data.get_srv(), data.get_path()), data);
			}

			break;

		case sml::CODE_ROOT_BROKER:
			execute_cmd_set_proc_param_broker(sml_gen, data, cyng::to_vector(params[0].get("brokers")));
			push_trx(sml_gen.get_proc_parameter(data.get_srv(), data.get_path()), data);
			break;

		case sml::CODE_ROOT_HARDWARE_PORT:
			execute_cmd_set_proc_param_hw_port(sml_gen, data, params.container());
			push_trx(sml_gen.get_proc_parameter(data.get_srv(), data.get_path()), data);
			break;

		default:
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> unknown OBIS code for SetProcParameterRequest: "
				<< data.get_root().to_str());
			break;
		}
	}

	void gateway_proxy::execute_cmd_get_list_request(sml::req_generator& sml_gen
		, proxy_data const& data
		, cyng::tuple_reader const& params)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> get "
			<< sml::from_server_id(data.get_srv())
			<< " GetListRequest: "
			<< data.get_root().to_str());

		//
		//	generate get process parameter requests
		//

		if (sml::OBIS_LIST_CURRENT_DATA_RECORD == data.get_root()) {

			auto const meter = cyng::value_cast<std::string>(params.get("meter"), "");
			auto const r = sml::parse_srv_id(meter);
			if (r.second) {
				push_trx(sml_gen.get_list_last_data_record(get_mac().to_buffer()
					, r.first), data);
			}
			else {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> invalid meter id: "
					<< meter);
			}
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_ipt(sml::req_generator& sml_gen
		, proxy_data const& data
		, std::uint8_t idx
		, cyng::tuple_t&& tpl)
	{
		for (auto const& val : tpl) {

			//	host
			//	port
			//	user
			//	pwd
			auto const param = cyng::to_param(val);

			if (boost::algorithm::equals(param.first, "host")) {

				auto address = cyng::value_cast<std::string>(param.second, "localhost");
				push_trx(sml_gen.set_proc_parameter_ipt_host(data.get_srv()
					, idx
					, address), data);
			}
			else if (boost::algorithm::equals(param.first, "port")) {
				auto port = cyng::numeric_cast<std::uint16_t>(param.second, 26862u);
				push_trx(sml_gen.set_proc_parameter_ipt_port_local(data.get_srv()
					, idx
					, port), data);
			}
			else if (boost::algorithm::equals(param.first, "user")) {
				auto str = cyng::value_cast<std::string>(param.second, "");
				push_trx(sml_gen.set_proc_parameter_ipt_user(data.get_srv()
					, idx
					, str), data);
			}
			else if (boost::algorithm::equals(param.first, "pwd")) {
				auto str = cyng::value_cast<std::string>(param.second, "");
				push_trx(sml_gen.set_proc_parameter_ipt_pwd(data.get_srv()
					, idx
					, str), data);
			}
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_wmbus(sml::req_generator& sml_gen
		, proxy_data const& data
		, cyng::tuple_t&& params)
	{
		for (auto const& obj : params) {

			cyng::param_t param;
			param = cyng::value_cast(obj, param);

			CYNG_LOG_DEBUG(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set w-MBus parameter "
				<< param.first
				<< ": "
				<< cyng::io::to_str(param.second));

			if (boost::algorithm::equals(param.first, "active")) {

				const auto b = cyng::value_cast(param.second, false);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_wMBUS, sml::OBIS_W_MBUS_INSTALL_MODE }
					, b), data);
			}
			else if (boost::algorithm::equals(param.first, "protocol")) {

				//
				//	protocol type is encodes as a single character
				//
				auto const val = cyng::value_cast<std::string>(param.second, "S");

				push_trx(sml_gen.set_proc_parameter_wmbus_protocol(data.get_srv()
					//
					//	function to convert the character into a numeric value
					//
					, [](std::string str)->std::uint8_t {
						if (!str.empty()) {
							switch (str.at(0)) {
							case 'S':	return 1;
							case 'A':	return 2;
							case 'P':	return 3;
							default:
								break;
							}
						}
						return 0;
					}(val)), data);

			}
			else if (boost::algorithm::equals(param.first, "reboot")) {

				const auto val = cyng::numeric_cast<std::uint64_t>(param.second, 0u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_wMBUS, sml::OBIS_W_MBUS_REBOOT }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "sMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 0u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_wMBUS, sml::OBIS_W_MBUS_MODE_S }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "tMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 0u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_wMBUS, sml::OBIS_W_MBUS_MODE_T }
					, val), data);
			}
		}
	}

	void gateway_proxy::execute_cmd_set_proc_param_iec(sml::req_generator& sml_gen
		, proxy_data const& data
		, cyng::tuple_t&& params)
	{
        //	active: false,
        //	autoActivation: true,
        //	loopTime: 3600,
        //	maxDataRate: 10240,
        //	minTimeout: 200,
        //	maxTimeout: 5000,
        //	maxVar: 9,
        //	protocolMode: 'C',
        //	retries: 3,
        //	rs485: null,
        //	timeGrid: 900,
        //	timeSync: 14400,
        //	devices: []
 

		for (auto const& obj : params) {

			cyng::param_t param;
			param = cyng::value_cast(obj, param);

			CYNG_LOG_DEBUG(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set IEC parameter "
				<< param.first
				<< ": "
				<< cyng::io::to_str(param.second));

			if (boost::algorithm::equals(param.first, "active")) {

				const auto b = cyng::value_cast(param.second, false);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_ACTIVE }
					, b), data);
			}
			else if (boost::algorithm::equals(param.first, "autoActivation")) {

				const auto b = cyng::value_cast(param.second, false);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_AUTO_ACTIVATION }
					, b), data);
			}
			else if (boost::algorithm::equals(param.first, "loopTime")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 60u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_LOOP_TIME }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "maxDataRate")) {

				const auto val = cyng::numeric_cast<std::uint64_t>(param.second, 0u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_MAX_DATA_RATE }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "minTimeout")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 200u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_MIN_TIMEOUT }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "maxTimeout")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 5000u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_MAX_TIMEOUT }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "maxVar")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 9u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_MAX_VARIATION }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "protocolMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 1u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_PROTOCOL_MODE }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "retries")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 3u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_RETRIES }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "rs485")) {
				//	ignore
			}
			else if (boost::algorithm::equals(param.first, "timeGrid")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 900u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_TIME_GRID }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "timeSync")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 14400u);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, sml::obis_path_t{ sml::OBIS_IF_1107, sml::OBIS_IF_1107_TIME_SYNC }
					, val), data);
			}
			else if (boost::algorithm::equals(param.first, "devices")) {
				
				//
				//	ToDo:
				//
			}
		}
	}

	bool gateway_proxy::execute_cmd_set_proc_param_meter(sml::req_generator& sml_gen
		, proxy_data const& data
		, std::string&& meter
		, std::string user
		, std::string pwd
		, std::string pubKey
		, std::string aesKey)
	{

		//
		//	parameters are "meterId" and "data" with 
		//	devClass: '-',
		//	maker: '-',
		//	status: 0,
		//	bitmask: '00 00',
		//	interval: 0,
		//	lastRecord: "1964-04-20",
		//	pubKey: '',
		//	aesKey: '',
		//	user: '',
		//	pwd: ''
		//	timeRef: 0
		//

		//
		//	meter ID is used as server ID
		//

		auto const r = cyng::parse_hex_string(meter);
		if (r.second) {


			//
			//	modify user name
			//
			push_trx(sml_gen.set_proc_parameter(r.first
				, { sml::OBIS_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_USER_NAME }
			, user), data);

			//
			//	modify user password
			//
			push_trx(sml_gen.set_proc_parameter(r.first
				, { sml::OBIS_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_USER_PWD }
			, pwd), data);

			//
			//	modify public key
			//
			//auto const new_pub_key = cyng::value_cast<std::string>(dom[1]["data"].get("pubKey"), "");
			auto const r_pub_key = cyng::parse_hex_string(pubKey);
			if (r_pub_key.second) {

				push_trx(sml_gen.set_proc_parameter(r.first
					, { sml::OBIS_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_PUBLIC_KEY }
				, r_pub_key.first), data);
			}

			//
			//	modify AES key
			//
			//auto const new_aes_key = cyng::value_cast<std::string>(dom[1]["data"].get("aesKey"), "");
			auto const r_aes_key = cyng::parse_hex_string(aesKey);
			if (r_aes_key.second) {

				push_trx(sml_gen.set_proc_parameter(r.first
					, { sml::OBIS_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_AES_KEY }
				, r_aes_key.first), data);
			}
		}
		else {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< data.get_root().to_str()
				<< " invalid meter id: "
				<< meter);
		}

		return r.second;
	}

	void gateway_proxy::execute_cmd_set_proc_param_broker(sml::req_generator& sml_gen
		, proxy_data const& data
		, cyng::vector_t&& params)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> set broker values: "
			<< cyng::io::to_type(params));

		//	example:
		//	[
		//	{("hardwarePort":COM3),("login":true),("addresses":[
		//		{("host":localhost),("service":12001),("user":w-MBus),("pwd":w-MBus)},
		//		{("host":segw.ch),("service":12001),("user":w-MBus),("pwd":w-MBus)}])},
		//	{("hardwarePort":COM6),("login":true),("addresses":[
		//		{("host":demo.com),("service":12007),("user":DEMO),("pwd":secret)}])}
		//	]


		std::uint8_t idxo{ 0 };
		for (auto const& p : params) {
			auto const port = cyng::to_param_map(p);
			try {

				++idxo;

				//
				//	login required
				//	
				auto const login = cyng::value_cast(port.at("login"), true);
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, { sml::OBIS_ROOT_BROKER, sml::make_obis(sml::OBIS_ROOT_BROKER, idxo), sml::OBIS_BROKER_LOGIN }
				, login), data);

				//
				//	name of hardware port
				//	
				auto const hardware_port = cyng::value_cast<std::string>(port.at("hardwarePort"), "");
				push_trx(sml_gen.set_proc_parameter(data.get_srv()
					, { sml::OBIS_ROOT_HARDWARE_PORT, sml::make_obis(sml::OBIS_HARDWARE_PORT_NAME, idxo) }
				, hardware_port), data);

				//
				//	target addresses
				//
				auto const addresses = cyng::to_vector(port.at("addresses"));

				std::uint8_t idxi{ 0 };
				for (auto const& a : addresses) {

					++idxi;
					auto const address = cyng::to_param_map(a);

					//
					//	host name
					//
					auto const host = cyng::value_cast<std::string>(address.at("host"), "");
					push_trx(sml_gen.set_proc_parameter(data.get_srv()
						, { sml::OBIS_ROOT_BROKER, sml::make_obis(sml::OBIS_ROOT_BROKER, idxo),  sml::make_obis(sml::OBIS_BROKER_SERVER, idxi) }
					, host), data);

					//
					//	IP port
					//
					auto const service = cyng::numeric_cast<std::uint16_t>(address.at("service"), 26862u + idxi);
					push_trx(sml_gen.set_proc_parameter(data.get_srv()
						, { sml::OBIS_ROOT_BROKER, sml::make_obis(sml::OBIS_ROOT_BROKER, idxo),  sml::make_obis(sml::OBIS_BROKER_SERVICE, idxi) }
					, service), data);

					//
					//	login name / account
					//
					auto const user = cyng::value_cast<std::string>(address.at("user"), "");
					push_trx(sml_gen.set_proc_parameter(data.get_srv()
						, { sml::OBIS_ROOT_BROKER, sml::make_obis(sml::OBIS_ROOT_BROKER, idxo),  sml::make_obis(sml::OBIS_BROKER_USER, idxi) }
					, user), data);

					//
					//	password
					//
					auto const pwd = cyng::value_cast<std::string>(address.at("pwd"), "");
					push_trx(sml_gen.set_proc_parameter(data.get_srv()
						, { sml::OBIS_ROOT_BROKER, sml::make_obis(sml::OBIS_ROOT_BROKER, idxo),  sml::make_obis(sml::OBIS_BROKER_PWD , idxi) }
					, pwd), data);
				}
			}
			catch(std::out_of_range const&) {}
		}

	}

	void gateway_proxy::execute_cmd_set_proc_param_hw_port(sml::req_generator& sml_gen
		, proxy_data const& data
		, cyng::tuple_t const& tpl)
	{
		//	//section 9100000000FF ==> 
		//	//	{
		//	//		("COM3":{
		//	//			("databits":8), 
		//	//			("parity":none), 
		//	//			("flowcontrol":none), 
		//	//			("stopbits":two), 
		//	//			("baudrate":57600)
		//	//			}
		//	//		)
		//	//	}
		//	for (auto const& obj : pd.get_params()) {
		//		auto const param = cyng::to_param(obj);
		//	}

		std::uint8_t idx{ 0 };
		for (auto const& obj : tpl) {
			auto const port = cyng::to_param(obj);

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> set hw port "
				<< port.first);

			auto const params = cyng::to_tuple(port.second);
			for (auto const& param : params) {

				auto const p = cyng::to_param(param);

				CYNG_LOG_DEBUG(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> set hw port "
					<< port.first
					<< " - "
					<< p.first
					<< ": "
					<< cyng::io::to_type(p.second));

				//HARDWARE_PORT_NAME
				//HARDWARE_PORT_DATABITS
				//HARDWARE_PORT_PARITY
				//HARDWARE_PORT_FLOW_CONTROL
				//HARDWARE_PORT_STOPBITS
				//HARDWARE_PORT_SPEED

				//push_trx(sml_gen.set_proc_parameter(data.get_srv()
				//	, { sml::OBIS_ ROOT_HARDWARE_PORT, sml::make_obis(sml::OBIS_HARDWARE_PORT_NAME, idx) }
				//, user), data);

			}
		}
	}



	bool gateway_proxy::execute_cmd_set_proc_param_activate(sml::req_generator& sml_gen
		, proxy_data const& data
		, std::uint8_t nr
		, std::string&& meter)
	{
		auto const r = cyng::parse_hex_string(meter);
		if (r.second) {
			push_trx(sml_gen.set_proc_parameter(data.get_srv()
				, { sml::OBIS_ACTIVATE_DEVICE, sml::make_obis(sml::OBIS_ACTIVATE_DEVICE, nr), sml::OBIS_SERVER_ID }
			, r.first), data);
		}
		else {
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> invalid meter id: "
				<< meter);
		}

		return r.second;
	}

	bool gateway_proxy::execute_cmd_set_proc_param_deactivate(sml::req_generator& sml_gen
		, proxy_data const& data
		, std::uint8_t nr
		, std::string&& meter)
	{
		auto const r = cyng::parse_hex_string(meter);
		if (r.second) {
			push_trx(sml_gen.set_proc_parameter(data.get_srv()
				, { sml::OBIS_DEACTIVATE_DEVICE, sml::make_obis(sml::OBIS_DEACTIVATE_DEVICE, nr), sml::OBIS_SERVER_ID }
			, r.first), data);
		}
		else {
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> invalid meter id: "
				<< meter);
		}
		return r.second;
	}

	bool gateway_proxy::execute_cmd_set_proc_param_delete(sml::req_generator& sml_gen
		, proxy_data const& data
		, std::uint8_t nr
		, std::string && meter)
	{
		auto const r = cyng::parse_hex_string(meter);

		if (r.second) {
			push_trx(sml_gen.set_proc_parameter(data.get_srv()
				, { sml::OBIS_DELETE_DEVICE, sml::make_obis(sml::OBIS_DELETE_DEVICE, nr), sml::OBIS_SERVER_ID }
			, r.first), data);
		}
		else {
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> invalid meter id: "
				<< meter);
		}
		return r.second;
	}

	void gateway_proxy::stop(bool shutdown)
	{
		//
		//	terminate task
		//
		//	Be careful when using resources from the session like vm_, 
		//	they may already be invalid.
		//
		auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_);
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped after "
			<< cyng::to_str(uptime));
	}

	void gateway_proxy::push_trx(std::string trx, proxy_data const& data)
	{
		auto const r = output_map_.emplace(trx, data);
		if (r.second) {
			CYNG_LOG_DEBUG(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> store trx "
				<< trx
				<< " => "
				<< sml::messages::name(data.get_msg_code()));
		}
		else {
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> store trx "
				<< trx
				<< " => "
				<< sml::messages::name(data.get_msg_code())
				<< " failed");
		}
	}

	void gateway_proxy::update_cfg_cache(std::string srv
		, sml::obis_path_t path
		, proxy_data const& pd
		, cyng::param_map_t const& params
		, bool force)
	{
		if (config_cache_.update(path, params, force)) {

			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> cache updated "
				<< sml::to_hex(path, ':')
				<< " => "
				<< cyng::io::to_str(params));
		}
		else {
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> "
				<< sml::to_hex(path, ':')
				<< " is not cached");

		}
	}

	void gateway_proxy::start_job_access_rights(boost::uuids::uuid tag,
		boost::uuids::uuid source,
		std::uint64_t seq,
		boost::uuids::uuid origin,
		cyng::vector_t pk,
		cyng::buffer_t srv_id,
		std::string name,
		std::string pwd)
	{
		//
		//	activate configuration cache
		//
		config_cache_.add(srv_id, sml::obis_path_t{ sml::OBIS_ROOT_ACCESS_RIGHTS });

		//
		//	query all active sensors/meters
		//
		//input_queue_.emplace(tag
		//	, source
		//	, seq
		//	, origin
		//	, "GetProcParameterRequest"
		//	, sml::obis_path_t({ sml::OBIS_ROOT_ACTIVE_DEVICES })
		//	, pk
		//	, cyng::tuple_t{}
		//	, srv_id
		//	, name
		//	, pwd
		//	, true);

		//
		//	query access rights of gateway
		//
		input_queue_.emplace(tag
			, source
			, seq
			, origin
			, "GetProcParameterRequest"
			, sml::obis_path_t({ sml::OBIS_ROOT_ACCESS_RIGHTS })
			, pk
			, cyng::tuple_t{}
			, srv_id
			, name
			, pwd
			, true);

		//
		//	after receiving the access rights of the gateway query 
		//	the individual rights on sensor/meters
		//
	}

	void gateway_proxy::process_job_result(sml::obis_path_t path, proxy_data const& prx, cyng::param_map_t const& params)
	{
		switch (path.front().to_uint64()) {
		case sml::CODE_ROOT_ACCESS_RIGHTS:

			if (path.size() == 1) {
				CYNG_LOG_TRACE(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> query rights now of "
					<< config_cache_.get_server());
				//
				//	send a message to this task (to synchronise access to input queue)
				//
				queue_access_right_queries(prx, params);
			}
			else {
				CYNG_LOG_TRACE(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> received access rights of "
					<< config_cache_.get_server()
					<< ", "
					//	contains the parameters index / total size / device nr 
					//	{("complete":true),("idx":5),("load":100),("nr":10),("total":6)}
					<< cyng::io::to_str(prx.get_params()));

				//
				//	send data back
				//
				auto const pm = cyng::to_param_map(prx.get_params());
				bus_->vm_.async_run(bus_res_com_proxy(prx.get_tag_ident()
					, prx.get_tag_source()
					, prx.get_sequence()
					, prx.get_key_gw()
					, prx.get_tag_origin()
					, "cache.update"
					, sml::from_server_id(prx.get_srv())
					, sml::path_to_vector(path)
					, pm));

				auto const complete = cyng::from_param_map(pm, "complete", false);
				if (complete) {

					//
					//	send complete data record
					//
					send_access_rights_from_cache(prx.get_tag_ident()
						, prx.get_tag_source()
						, prx.get_sequence()
						, prx.get_tag_origin()
						, prx.get_key_gw()
						, prx.get_srv()
						, prx.get_user()
						, prx.get_pwd());
				}
			}
			break;
		default:
			if (params.empty()) {
				CYNG_LOG_WARNING(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> no active devices on "
					<< config_cache_.get_server());
			}
			//
			//	send data back
			//
			bus_->vm_.async_run(bus_res_com_proxy(prx.get_tag_ident()
				, prx.get_tag_source()
				, prx.get_sequence()
				, prx.get_key_gw()
				, prx.get_tag_origin()
				, "cache.update"
				, sml::from_server_id(prx.get_srv())
				, sml::path_to_vector(path)
				, params));
			break;
		}
	}

	void gateway_proxy::queue_access_right_queries(proxy_data const& prx, cyng::param_map_t const& params)
	{
		config_cache_.loop_access_rights([&](std::uint8_t role, std::uint8_t user, std::string name, config_cache::device_map_t devices) {

			std::size_t counter{ 0 };
			for (auto const& dev : devices) {

				CYNG_LOG_TRACE(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> get all access rights now of role #"
					<< +role
					<< ", user #"
					<< +user
					<< " ("
					<< name
					<< "), sensor: #"
					<< dev.first.get_storage()
					<< " - "
					<< sml::from_server_id(dev.second));

				auto const pm = cyng::param_map_factory("idx", counter)
					("total", devices.size())
					("complete", ((counter + 1) == devices.size()))
					("load", (((counter + 1) *100) /  devices.size()))
					("nr", dev.first.get_storage()).operator cyng::param_map_t();
				place_access_right_request(prx.get_tag_ident()
					, prx.get_tag_source()
					, prx.get_sequence()
					, prx.get_tag_origin()
					, role
					, user
					, dev.first.get_storage()
					, prx.get_key_gw()
					, prx.get_srv()
					, prx.get_user()
					, prx.get_pwd()
					, cyng::to_tuple(pm));

				++counter;
			}
		});
	}

	void gateway_proxy::place_access_right_request(boost::uuids::uuid tag
		, boost::uuids::uuid source
		, std::uint64_t seq
		, boost::uuids::uuid origin
		, std::uint8_t role
		, std::uint8_t	user
		, std::uint8_t meter
		, cyng::vector_t gw
		, cyng::buffer_t srv_id
		, std::string name
		, std::string pwd
		, cyng::tuple_t&& params)
	{
		input_queue_.emplace(tag
			, source
			, seq
			, origin
			, "GetProcParameterRequest"
			, sml::obis_path_t({ sml::OBIS_ROOT_ACCESS_RIGHTS	//	81 81 81 60 FF FF
				, sml::make_obis(0x81, 0x81, 0x81, 0x60, role, 0xFF)
				, sml::make_obis(0x81, 0x81, 0x81, 0x60, role, user)
				, sml::make_obis(0x81, 0x81, 0x81, 0x64, 0x01, meter)
				})
			, gw
			, params
			, srv_id
			, name
			, pwd
			, true);	//	job
	}

	void gateway_proxy::send_access_rights_from_cache(boost::uuids::uuid tag,
		boost::uuids::uuid source,
		std::uint64_t seq,
		boost::uuids::uuid origin,
		cyng::vector_t pk,
		cyng::buffer_t srv_id,
		std::string name,
		std::string pwd)
	{
		config_cache_.loop_access_rights([&](std::uint8_t role, std::uint8_t user, std::string name, config_cache::device_map_t devices) {

			std::uint16_t counter{ 0 };
			for (auto const& dev : devices) {

				CYNG_LOG_TRACE(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> send access right of role #"
					<< +role
					<< ", user #"
					<< +user
					<< " ("
					<< name
					<< "), meter: #"
					<< dev.first.get_storage()
					<< " - "
					<< sml::from_server_id(dev.second));

				auto const path = sml::obis_path_t({ sml::OBIS_ROOT_ACCESS_RIGHTS	//	81 81 81 60 FF FF
					, sml::make_obis(0x81, 0x81, 0x81, 0x60, role, 0xFF)
					, sml::make_obis(0x81, 0x81, 0x81, 0x60, role, user)
					, sml::make_obis(0x81, 0x81, 0x81, 0x64, 0x01, dev.first.get_storage())
					});
				auto section = config_cache_.get_section(path);

				if (!section.empty()) {

					//	"81818161FFFF"
					section[sml::OBIS_ACCESS_USER_NAME.to_str()] = cyng::make_object(name);
					section["role"] = cyng::make_object(role);
					section["nr"] = cyng::make_object(dev.first.get_storage());
					section["counter"] = cyng::make_object(counter);
					section["total"] = cyng::make_object(devices.size());

					bus_->vm_.async_run(bus_res_com_proxy(tag
						, source
						, seq
						, pk
						, origin
						, "cache.query"
						, sml::from_server_id(srv_id)
						, sml::path_to_vector(path)
						, section));

					++counter;
				}
			}
		});
	}

	cyng::mac48 get_mac()
	{
		auto const macs = cyng::sys::retrieve_mac48();

		return (macs.empty())
			? macs.front()
			: cyng::generate_random_mac48()
			;
	}

	cyng::param_map_t transform_status_word(cyng::object obj)
	{
		sml::status_word_t word{ 0 };
		sml::status wrapper(word);
		wrapper.reset(cyng::numeric_cast<std::uint32_t>(obj, 0u));

		return cyng::param_map_factory("word", sml::to_param_map(word));
	}


	cyng::param_map_t transform_broker_params(cyng::logging::log_ptr logger, cyng::param_map_t const& pm)
	{
		BOOST_ASSERT_MSG(pm.size() % 2 == 0, "invalid broker data");

		//
		//	collect all broker
		//
		cyng::vector_t brokers;

		for (std::uint8_t idx_pm = 1; idx_pm < pm.size() + 1; ++idx_pm) {

			auto pos_broker = pm.find(sml::make_obis(sml::OBIS_ROOT_BROKER, idx_pm).to_str());
			if (pos_broker != pm.end()) {

				auto const targets = cyng::to_param_map(pos_broker->second);
				//BOOST_ASSERT_MSG((targets.size() - 1) % 4 == 0, "invalid broker target data");

				auto pos_port = targets.find(sml::OBIS_HARDWARE_PORT_NAME.to_str());
				if (pos_port != targets.end()) {
					auto pos_login = targets.find(sml::OBIS_BROKER_LOGIN.to_str());
					if (pos_login != targets.end()) {

						//
						//	collect all addresses of this broker
						//
						cyng::vector_t addresses;

						for (std::uint8_t idxi = 1; idxi < (targets.size() / 4) + 1; ++idxi) {
							auto pos_host = targets.find(sml::make_obis(sml::OBIS_BROKER_SERVER, idxi).to_str());
							if (pos_host != targets.end()) {
								auto pos_service = targets.find(sml::make_obis(sml::OBIS_BROKER_SERVICE, idxi).to_str());
								if (pos_service != targets.end()) {
									auto pos_user = targets.find(sml::make_obis(sml::OBIS_BROKER_USER, idxi).to_str());
									if (pos_user != targets.end()) {
										auto pos_pwd = targets.find(sml::make_obis(sml::OBIS_BROKER_PWD, idxi).to_str());
										if (pos_pwd != targets.end()) {

											//
											//	addresses complete - build broker tuple
											//
											auto const tpl = cyng::tuple_factory(
												cyng::param_t("host", pos_host->second),
												cyng::param_t("service", pos_service->second),
												cyng::param_t("user", pos_user->second),
												cyng::param_t("pwd", pos_pwd->second)
											);

											addresses.push_back(cyng::make_object(tpl));

										}
									}
								}
							}
						}

						//
						//	addresses complete - build broker tuple
						//
						auto const tpl = cyng::tuple_factory(
							cyng::param_t("hardwarePort", pos_port->second),
							cyng::param_t("login", pos_login->second),
							cyng::param_factory("addresses", addresses)
						);

						brokers.push_back(cyng::make_object(tpl));

					}
				}
				//if (pos_port != pm.end()) {
				//	auto pos_targets = pm.find(sml::make_obis(0x90, 0x00, 0x00, 0x00, 0x03, idxo).to_str());
				//	if (pos_targets != pm.end()) {
				//		auto const targets = cyng::to_param_map(pos_targets->second);
				//		BOOST_ASSERT_MSG(targets.size() % 4 == 0, "invalid broker target data");

				//		//
				//		//	collect all addresses of this broker
				//		//
				//		cyng::vector_t addresses;

				//		for (std::uint8_t idxi = 1; idxi < (targets.size() / 4) + 1; ++idxi) {
				//			auto pos_host = targets.find(sml::make_obis(0x90, 0x00, 0x00, 0x00, 0x03, idxi).to_str());
				//			if (pos_host != targets.end()) {
				//				auto pos_service = targets.find(sml::make_obis(0x90, 0x00, 0x00, 0x00, 0x04, idxi).to_str());
				//				if (pos_service != targets.end()) {
				//					auto pos_user = targets.find(sml::make_obis(0x90, 0x00, 0x00, 0x00, 0x05, idxi).to_str());
				//					if (pos_user != targets.end()) {
				//						auto pos_pwd = targets.find(sml::make_obis(0x90, 0x00, 0x00, 0x00, 0x06, idxi).to_str());
				//						if (pos_pwd != targets.end()) {

				//							//
				//							//	broker data complete
				//							//
				//							auto const tpl = cyng::tuple_factory(
				//								cyng::param_t("host", pos_host->second),
				//								cyng::param_t("service", pos_service->second),
				//								cyng::param_t("user", pos_user->second),
				//								cyng::param_t("pwd", pos_pwd->second)
				//							);

				//							addresses.push_back(cyng::make_object(tpl));

				//						}
				//					}
				//				}
				//			}
				//		}	//	idxi loop

				//		//
				//		//	addresses complete - build broker tuple
				//		//
				//		auto const tpl = cyng::tuple_factory(
				//			cyng::param_t("hardwarePort", pos_port->second),
				//			cyng::param_t("login", pos_login->second),
				//			cyng::param_factory("addresses", addresses)
				//		);

				//		brokers.push_back(cyng::make_object(tpl));

				//	}
				//}
			}
			else {
				CYNG_LOG_WARNING(logger, "missing login data: "
					<< cyng::io::to_type(pm));
			}
		}	//	idx loop

		//CYNG_LOG_DEBUG(logger, "broker: "
		//	<< cyng::io::to_type(brokers));

		return cyng::param_map_factory("brokers", brokers);
	}

	cyng::param_map_t transform_broker_hw_params(cyng::logging::log_ptr logger, cyng::param_map_t const& pm)
	{
		//	%(("910000000101":COM3),("910000000102":COM6),("910000000201":8),("910000000202":8),("910000000301":none),("910000000302":even),("910000000401":none),("910000000402":none),("910000000501":one),("910000000502":one),("910000000601":e100),("910000000602":0960))

		BOOST_ASSERT_MSG(pm.size() % 6 == 0, "invalid broker hardware data");

		//name: {}

		cyng::param_map_t result;
		for (std::uint8_t idx = 1; idx < (pm.size() / 6) + 1; ++idx) {

			//	port name
			auto pos = pm.find(sml::make_obis(sml::OBIS_HARDWARE_PORT_NAME, idx).to_str());
			if (pos != pm.end()) {

				auto const name = cyng::value_cast<std::string>(pos->second, "");

				auto pos_databits = pm.find(sml::make_obis(sml::OBIS_HARDWARE_PORT_DATABITS, idx).to_str());
				if (pos_databits != pm.end()) {
					auto pos_parity = pm.find(sml::make_obis(sml::OBIS_HARDWARE_PORT_PARITY, idx).to_str());
					if (pos_parity != pm.end()) {
						auto pos_flow_control = pm.find(sml::make_obis(sml::OBIS_HARDWARE_PORT_FLOW_CONTROL, idx).to_str());
						if (pos_flow_control != pm.end()) {
							auto pos_stopbits = pm.find(sml::make_obis(sml::OBIS_HARDWARE_PORT_STOPBITS, idx).to_str());
							if (pos_stopbits != pm.end()) {
								auto pos_speed = pm.find(sml::make_obis(sml::OBIS_HARDWARE_PORT_SPEED, idx).to_str());

								//
								//	serial port complete - build tuple
								//
								auto const tpl = cyng::tuple_factory(
									cyng::param_t("databits", pos_databits->second),
									cyng::param_t("parity", pos_parity->second),
									cyng::param_t("flowcontrol", pos_flow_control->second),
									cyng::param_t("stopbits", pos_stopbits->second),
									cyng::param_t("baudrate", pos_speed->second));

								result.emplace(name, cyng::make_object(tpl));

							}
						}
					}
				}

			}
		}

		return result;
	}

	cyng::param_map_t transform_data_collector_params(cyng::logging::log_ptr logger, cyng::param_map_t const& collectors)
	{
		//	8181C78620[NN]
		//	->	8181C78621FF
		//	->	8181C78622FF
		//	->	8181C78781FF
		//	->	8181C78A23FF
		//		-> list of registers (OBIS code 8181C78A23[NN])
		//	->	8181C78A83FF

		cyng::param_map_t result;

		for (auto const& collector : collectors) {
			auto const pm = cyng::to_param_map(collector.second);
			cyng::param_map_t params;
			for (auto const& entry : pm) {
				if (boost::algorithm::equals(entry.first, sml::OBIS_DATA_COLLECTOR_REGISTER.to_str())) {
					auto const regs = cyng::to_param_map(entry.second);
					cyng::vector_t vec;
					for (auto const& reg : regs) {
						vec.push_back(reg.second);
					}
					params[entry.first] = cyng::make_object(vec);
				}
				else {
					params[entry.first] = entry.second;
				}
			}
			result[collector.first] = cyng::make_object(params);
		}
		return result;
	}

}

#include <cyng/async/task/task.hpp>

namespace cyng {
	namespace async {

		//
		//	initialize static slot names
		//
		template <>
		std::map<std::string, std::size_t> cyng::async::task<node::gateway_proxy>::slot_names_({ 
			{ "ack", 0 },
			{ "shutdown", 1 }
		});

	}
}
