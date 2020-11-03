/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "proxy_data.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/parser/srv_id_parser.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/sml/protocol/generator.h>
#include <smf/sml/protocol/serializer.h>
#include <smf/sml/crc16.h>

#include <cyng/vector_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/dom/reader.h>
#include <cyng/parser/buffer_parser.h>
#include <cyng/factory/factory.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/sys/mac.h>

#include <boost/algorithm/string.hpp>

namespace node
{

	reply_data::reply_data(
		proxy::cluster_data&& cd,
		proxy::sml_data&& sd,
		sml::obis_path_t path,
		bool job	//	job - true if running as job
	)
		: cluster_data_(std::move(cd))
		, sml_data_(std::move(sd))
		, path_(path)	//	root path
		, job_(job)
	{}

	reply_data reply_data::copy() const
	{
		return reply_data(cluster_data_.copy()
			, sml_data_.copy()
			, path_
			, job_);
	}

	sml::obis_path_t const& reply_data::get_path() const
	{
		return path_;
	}

	bool reply_data::is_job() const
	{
		return job_;
	}


	proxy::cluster_data const& reply_data::get_cluster_data() const
	{
		return cluster_data_;
	}

	proxy::sml_data const& reply_data::get_sml_data() const
	{
		return sml_data_;
	}

	//
	//	helper functions:
	//


	cyng::object read_server_id(cyng::object obj)
	{
		auto const inp = cyng::value_cast<std::string>(obj, "");
		auto const r = cyng::parse_hex_string(inp);
		return (r.second)
			? cyng::make_object(r.first)
			: cyng::make_object()
			;
	}

	cyng::object parse_server_id(cyng::object obj)
	{
		auto const inp = cyng::value_cast<std::string>(obj, "");
		std::pair<cyng::buffer_t, bool> const r = node::sml::parse_srv_id(inp);
		return (r.second)
			? cyng::make_object(r.first)
			: cyng::make_object()
			;
	}

	cyng::tuple_t set_proc_parameter_ipt_param(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
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

				auto host = cyng::value_cast<std::string>(param.second, "localhost");
				sml::merge_msg(msg
					, { sml::OBIS_ROOT_IPT_PARAM, sml::make_obis(sml::OBIS_ROOT_IPT_PARAM, idx), sml::make_obis(sml::OBIS_TARGET_IP_ADDRESS, idx) }
				, node::sml::make_value(host));	//	ip address of IP-T master 
			}
			else if (boost::algorithm::equals(param.first, "port")) {
				auto port = cyng::numeric_cast<std::uint16_t>(param.second, 26862u);
				sml::merge_msg(msg
					, { sml::OBIS_ROOT_IPT_PARAM, sml::make_obis(sml::OBIS_ROOT_IPT_PARAM, idx), sml::make_obis(sml::OBIS_TARGET_PORT, idx) }
				, node::sml::make_value(port));
			}
			else if (boost::algorithm::equals(param.first, "user")) {
				auto user = cyng::value_cast<std::string>(param.second, "");
				sml::merge_msg(msg
					, { sml::OBIS_ROOT_IPT_PARAM, sml::make_obis(sml::OBIS_ROOT_IPT_PARAM, idx), sml::make_obis(sml::OBIS_IPT_ACCOUNT, idx) }
				, node::sml::make_value(user));
			}
			else if (boost::algorithm::equals(param.first, "pwd")) {
				auto pwd = cyng::value_cast<std::string>(param.second, "");
				sml::merge_msg(msg
					, { sml::OBIS_ROOT_IPT_PARAM, sml::make_obis(sml::OBIS_ROOT_IPT_PARAM, idx), sml::make_obis(sml::OBIS_IPT_PASSWORD, idx) }
				, node::sml::make_value(pwd));
			}

#ifdef __DEBUG
			std::cerr
				<< cyng::io::to_type(msg)
				<< std::endl;
#endif
		}

		return msg;
	}

	cyng::tuple_t set_proc_parameter_activate_device(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, std::uint8_t idx
		, std::string&& meter)
	{
		auto const r = cyng::parse_hex_string(meter);
		if (r.second) {

			sml::merge_msg(msg
				, { sml::OBIS_ACTIVATE_DEVICE, sml::make_obis(sml::OBIS_ACTIVATE_DEVICE, idx), sml::OBIS_SERVER_ID }
			, node::sml::make_value(r.first));

		}
		return msg;
	}

	cyng::tuple_t set_proc_parameter_deactivate_device(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, std::uint8_t idx
		, std::string&& meter)
	{
		auto const r = cyng::parse_hex_string(meter);
		if (r.second) {

			sml::merge_msg(msg
				, { sml::OBIS_DEACTIVATE_DEVICE, sml::make_obis(sml::OBIS_DEACTIVATE_DEVICE, idx), sml::OBIS_SERVER_ID }
			, node::sml::make_value(r.first));

		}
		return msg;
	}

	cyng::tuple_t set_proc_parameter_delete_device(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, std::uint8_t idx
		, std::string&& meter)
	{
		auto const r = cyng::parse_hex_string(meter);
		if (r.second) {

			sml::merge_msg(msg
				, { sml::OBIS_DELETE_DEVICE, sml::make_obis(sml::OBIS_DELETE_DEVICE, idx), sml::OBIS_SERVER_ID }
			, node::sml::make_value(r.first));

		}
		return msg;
	}

	cyng::tuple_t set_proc_parameter_sensor_params(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, std::string&& user
		, std::string&& pwd
		, std::string&& pub_key
		, std::string&& aes_key)
	{
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

		sml::merge_msg(msg
			, { sml::OBIS_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_USER_NAME }
		, node::sml::make_value(user));

		sml::merge_msg(msg
			, { sml::OBIS_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_USER_PWD }
		, node::sml::make_value(pwd));

		//
		//	modify public key
		//
		auto const r_pub_key = cyng::parse_hex_string(pub_key);
		if (r_pub_key.second) {
			sml::merge_msg(msg
				, { sml::OBIS_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_PUBLIC_KEY }
			, node::sml::make_value(r_pub_key.first));
		}

		//
		//	modify AES key
		//
		auto const r_aes_key = cyng::parse_hex_string(aes_key);
		if (r_aes_key.second) {
			sml::merge_msg(msg
				, { sml::OBIS_ROOT_SENSOR_PARAMS, sml::OBIS_DATA_AES_KEY }
			, node::sml::make_value(r_aes_key.first));
		}

		return msg;
	}

	cyng::tuple_t set_proc_parameter_broker(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, cyng::vector_t&& brokers)
	{
		//"brokers": [
		//{
		//	"hardwarePort": "COM3",
		//		"login" : true,
		//		"addresses" : [
		//	{
		//		"host": "localhost",
		//			"service" : 12001,
		//			"user" : "VVjWxG",
		//			"pwd" : "2Uo3dFb9"
		//	}
		//		]
		//},
		//	{
		//		"hardwarePort": "COM6",
		//		"login" : true,
		//		"addresses" : [
		//			{
		//				"host": "localhost",
		//				"service" : 12002,
		//				"user" : "uh0Jx6",
		//				"pwd" : "Gm8o6Zkq"
		//			}
		//		]
		//	}
		//]

		std::uint8_t idx_broker{ 0 };
		for (auto const& obj : brokers) {
			auto const broker = cyng::to_param_map(obj);

			++idx_broker;

			try {
				//
				//	login required
				//	
				auto const login = cyng::value_cast(broker.at("login"), true);
				sml::merge_msg(msg
					, { sml::OBIS_ROOT_BROKER, sml::make_obis(sml::OBIS_ROOT_BROKER, idx_broker), sml::OBIS_BROKER_LOGIN }
				, node::sml::make_value(login));

				//
				//	name of hardware port
				//	different root (OBIS_ROOT_SERIAL)
				//	ToDo: separate message required
				//	
				//auto const hardware_port = cyng::value_cast<std::string>(broker.at("hardwarePort"), "");
				//sml::merge_msg(msg
				//	, { sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_SERIAL_NAME, idx_broker) }
				//, node::sml::make_value(hardware_port));

				//
				//	target addresses
				//
				auto const addresses = cyng::to_vector(broker.at("addresses"));
				std::uint8_t idx_address{ 0 };
				for (auto const& a : addresses) {

					++idx_address;	//	starts with 1
					auto const address = cyng::to_param_map(a);

					//
					//	host name
					//
					auto const host = cyng::value_cast<std::string>(address.at("host"), "");
					sml::merge_msg(msg
						, { sml::OBIS_ROOT_BROKER, sml::make_obis(sml::OBIS_ROOT_BROKER, idx_broker), sml::make_obis(sml::OBIS_BROKER_SERVER, idx_address) }
					, node::sml::make_value(host));

					//
					//	IP port
					//
					auto const service = cyng::numeric_cast<std::uint16_t>(address.at("service"), 26862u + idx_address);
					sml::merge_msg(msg
						, { sml::OBIS_ROOT_BROKER, sml::make_obis(sml::OBIS_ROOT_BROKER, idx_broker), sml::make_obis(sml::OBIS_BROKER_SERVICE, idx_address) }
					, node::sml::make_value(service));

					//
					//	login name / account
					//
					auto const user = cyng::value_cast<std::string>(address.at("user"), "");
					sml::merge_msg(msg
						, { sml::OBIS_ROOT_BROKER, sml::make_obis(sml::OBIS_ROOT_BROKER, idx_broker), sml::make_obis(sml::OBIS_BROKER_USER, idx_address) }
					, node::sml::make_value(user));

					//
					//	password
					//
					auto const pwd = cyng::value_cast<std::string>(address.at("pwd"), "");
					sml::merge_msg(msg
						, { sml::OBIS_ROOT_BROKER, sml::make_obis(sml::OBIS_ROOT_BROKER, idx_broker), sml::make_obis(sml::OBIS_BROKER_PWD, idx_address) }
					, node::sml::make_value(pwd));


				}


			}
			catch (std::out_of_range const&) {}
		}

		return msg;
	}

	cyng::tuple_t set_proc_parameter_if_wmbus(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, cyng::tuple_t&& tpl)
	{
		for (auto const& obj : tpl) {

			auto const param = cyng::to_param(obj);

			if (boost::algorithm::equals(param.first, "active")) {

				const auto b = cyng::value_cast(param.second, false);
				sml::merge_msg(msg
					, { sml::OBIS_IF_wMBUS, sml::OBIS_W_MBUS_INSTALL_MODE }
				, node::sml::make_value(b));
			}
			else if (boost::algorithm::equals(param.first, "protocol")) {

				//
				//	protocol type is encodes as a single character
				//
				auto const val = cyng::value_cast<std::string>(param.second, "S");

				//
				//	function to convert the character into a numeric value
				//
				auto f_conv = [](std::string str)->std::uint8_t {
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
				};
				sml::merge_msg(msg
					, { sml::OBIS_IF_wMBUS, sml::OBIS_W_MBUS_PROTOCOL }
				, node::sml::make_value(f_conv(val)));
			}
			else if (boost::algorithm::equals(param.first, "reboot")) {

				const auto val = cyng::numeric_cast<std::uint64_t>(param.second, 0u);
				sml::merge_msg(msg
					, { sml::OBIS_IF_wMBUS, sml::OBIS_W_MBUS_REBOOT }
				, node::sml::make_value(val));
			}
			else if (boost::algorithm::equals(param.first, "sMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 0u);
				sml::merge_msg(msg
					, { sml::OBIS_IF_wMBUS, sml::OBIS_W_MBUS_MODE_S }
				, node::sml::make_value(val));
			}
			else if (boost::algorithm::equals(param.first, "tMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 0u);
				sml::merge_msg(msg
					, { sml::OBIS_IF_wMBUS, sml::OBIS_W_MBUS_MODE_T }
				, node::sml::make_value(val));
			}
		}
		return msg;
	}

	cyng::tuple_t set_proc_parameter_if_1107(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, cyng::tuple_t&& tpl)
	{
		//       //	active: false,
		//       //	autoActivation: true,
		//       //	loopTime: 3600,
		//       //	maxDataRate: 10240,
		//       //	minTimeout: 200,
		//       //	maxTimeout: 5000,
		//       //	maxVar: 9,
		//       //	protocolMode: 'C',
		//       //	retries: 3,
		//       //	rs485: null,
		//       //	timeGrid: 900,
		//       //	timeSync: 14400,
		//       //	devices: []
		//

		for (auto const& obj : tpl) {

			auto const param = cyng::to_param(obj);

			if (boost::algorithm::equals(param.first, "active")) {

				const auto b = cyng::value_cast(param.second, false);
				sml::merge_msg(msg
					, { sml::OBIS_IF_1107, sml::OBIS_IF_1107_ACTIVE }
				, node::sml::make_value(b));

			}
			else if (boost::algorithm::equals(param.first, "autoActivation")) {

				const auto b = cyng::value_cast(param.second, false);
				sml::merge_msg(msg
					, { sml::OBIS_IF_1107, sml::OBIS_IF_1107_AUTO_ACTIVATION }
				, node::sml::make_value(b));

			}
			else if (boost::algorithm::equals(param.first, "loopTime")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 60u);
				sml::merge_msg(msg
					, { sml::OBIS_IF_1107, sml::OBIS_IF_1107_LOOP_TIME }
				, node::sml::make_value(val));
			}
			else if (boost::algorithm::equals(param.first, "maxDataRate")) {

				const auto val = cyng::numeric_cast<std::uint64_t>(param.second, 0u);
				sml::merge_msg(msg
					, { sml::OBIS_IF_1107, sml::OBIS_IF_1107_MAX_DATA_RATE }
				, node::sml::make_value(val));
			}
			else if (boost::algorithm::equals(param.first, "minTimeout")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 200u);
				sml::merge_msg(msg
					, { sml::OBIS_IF_1107, sml::OBIS_IF_1107_MIN_TIMEOUT }
				, node::sml::make_value(val));
			}
			else if (boost::algorithm::equals(param.first, "maxTimeout")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 5000u);
				sml::merge_msg(msg
					, { sml::OBIS_IF_1107, sml::OBIS_IF_1107_MAX_TIMEOUT }
				, node::sml::make_value(val));
			}
			else if (boost::algorithm::equals(param.first, "maxVar")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 9u);
				sml::merge_msg(msg
					, { sml::OBIS_IF_1107, sml::OBIS_IF_1107_MAX_VARIATION }
				, node::sml::make_value(val));
			}
			else if (boost::algorithm::equals(param.first, "protocolMode")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 1u);
				sml::merge_msg(msg
					, { sml::OBIS_IF_1107, sml::OBIS_IF_1107_PROTOCOL_MODE }
				, node::sml::make_value(val));
			}
			else if (boost::algorithm::equals(param.first, "retries")) {

				const auto val = cyng::numeric_cast<std::uint8_t>(param.second, 3u);
				sml::merge_msg(msg
					, { sml::OBIS_IF_1107, sml::OBIS_IF_1107_RETRIES }
				, node::sml::make_value(val));
			}
			else if (boost::algorithm::equals(param.first, "rs485")) {
				//	ignore
			}
			else if (boost::algorithm::equals(param.first, "timeGrid")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 900u);
				sml::merge_msg(msg
					, { sml::OBIS_IF_1107, sml::OBIS_IF_1107_TIME_GRID }
				, node::sml::make_value(val));
			}
			else if (boost::algorithm::equals(param.first, "timeSync")) {

				const auto val = cyng::numeric_cast<std::uint32_t>(param.second, 14400u);
				sml::merge_msg(msg
					, { sml::OBIS_IF_1107, sml::OBIS_IF_1107_TIME_SYNC }
				, node::sml::make_value(val));
			}
			else if (boost::algorithm::equals(param.first, "devices")) {

				//
				//	ToDo:
				//
			}
		}
		return msg;
	}

	proxy::data finalize_get_proc_parameter_request(
		proxy::cluster_data&& cd,
		proxy::sml_data&& sd,
		sml::obis_path_t&& path,		//	OBIS path
		cyng::tuple_reader const& reader,
		bool cache_enabled)
	{
		//
		//	generate SML messages
		//
		//	open request
		//	get proc parameter messages
		//	close request
		//
		node::sml::req_generator sml_gen(sd.name_, sd.pwd_);
		sml::messages_t messages{
			sml_gen.public_open(get_mac(), sd.srv_),
			sml_gen.get_proc_parameter(sd.srv_, path),
			sml_gen.public_close()
		};

		BOOST_ASSERT(messages.size() == 3);

		return proxy::make_data(std::move(messages)
			, std::move(cd)
			, std::move(sd)
			, path
			, cache_enabled);
	}

	proxy::data finalize_get_proc_parameter_request(
		proxy::cluster_data&& cd,
		proxy::sml_data&& sd,
		sml::obis root,				//	OBIS root code
		cyng::tuple_t const& params,
		bool cache_enabled)
	{
		auto const reader = cyng::make_reader(params);
		if (sml::OBIS_ROOT_ACCESS_RIGHTS == root) {

			auto const obj_path = cyng::to_vector(reader.get("path"));
			if (obj_path.size() == 3) {

				//
				//	build an OBIS path
				//
				auto const role = cyng::numeric_cast<std::uint8_t>(obj_path.at(0), 1);
				auto const user = cyng::numeric_cast<std::uint8_t>(obj_path.at(1), 1);
				auto const meter = cyng::numeric_cast<std::uint8_t>(obj_path.at(2), 1);

				return finalize_get_proc_parameter_request(
					std::move(cd),
					std::move(sd),
					sml::obis_path_t({ root
					, sml::make_obis(0x81, 0x81, 0x81, 0x60, role, 0xFF)
					, sml::make_obis(0x81, 0x81, 0x81, 0x60, role, user)
					, sml::make_obis(0x81, 0x81, 0x81, 0x64, 0x01, meter) }),				//	OBIS root code
					reader,
					cache_enabled);

			}
		}
		else if (sml::OBIS_ROOT_SENSOR_PARAMS == root
			|| sml::OBIS_ROOT_DATA_COLLECTOR == root
			|| sml::OBIS_ROOT_PUSH_OPERATIONS == root) {

			//
			//	communication with the meter - replace server id with
			//	meter id.
			//
			auto const meter = cyng::value_cast<std::string>(reader.get("meter"), "");
			auto const r = sml::parse_srv_id(meter);
			if (r.second) {
				return finalize_get_proc_parameter_request(
					std::move(cd),
					proxy::sml_data(sd, r.first),
					sml::obis_path_t({ root }),				//	OBIS root code
					reader,
					cache_enabled);

			}
		}

		return finalize_get_proc_parameter_request(
			std::move(cd),
			std::move(sd),
			sml::obis_path_t({ root }),				//	OBIS root code
			reader,
			cache_enabled);

	}

	proxy::data finalize_set_proc_parameter_request(
		proxy::cluster_data&& cd,
		proxy::sml_data&& sd,
		sml::obis_path_t&& path,		//	OBIS path
		cyng::tuple_reader const& reader,
		bool cache_enabled)
	{
		//
		//	SML request generator
		//
		node::sml::req_generator sml_gen(sd.name_, sd.pwd_);

		//
		//	initialize with open message
		//
		sml::messages_t messages{ sml_gen.public_open(get_mac(), sd.srv_) };

		switch (path.front().to_uint64()) {
		case sml::CODE_ROOT_IPT_PARAM:
			//
			//	modify IP-T parameters
			//
			messages.push_back(set_proc_parameter_ipt_param(sml_gen.empty_set_proc_param(sd.srv_, path.front()),
				sd.srv_,
				cyng::numeric_cast<std::uint8_t>(reader.get("index"), 0u),
				cyng::to_tuple(reader.get("ipt"))));
			break;
		case sml::CODE_IF_wMBUS:
			//
			//	modify wireless IEC parameters
			//
			messages.push_back(set_proc_parameter_if_wmbus(sml_gen.empty_set_proc_param(sd.srv_, path.front()),
				sd.srv_,
				cyng::to_tuple(reader.get("wmbus"))));
			break;
		case sml::CODE_IF_1107:
			//
			//	modify wired IEC parameters
			//
			messages.push_back(set_proc_parameter_if_1107(sml_gen.empty_set_proc_param(sd.srv_, path.front()),
				sd.srv_,
				cyng::to_tuple(reader.get("iec"))));
			break;
		case sml::CODE_REBOOT:
			//
			//	reboot the gateway.
			//	A "get process parameter request" doesn't make sense after reboot
			//
			messages.push_back(sml_gen.set_proc_parameter_reboot(sd.srv_));
			break;
		case sml::CODE_ACTIVATE_DEVICE:
			//
			//	activate meter device
			//
			messages.push_back(set_proc_parameter_activate_device(sml_gen.empty_set_proc_param(sd.srv_, path.front()),
				sd.srv_,
				cyng::numeric_cast<std::uint8_t>(reader.get("nr"), 0u),
				cyng::value_cast<std::string>(reader.get("meter"), "")));
			break;
		case sml::CODE_DEACTIVATE_DEVICE:
			//
			//	deactivate meter device
			//
			messages.push_back(set_proc_parameter_deactivate_device(sml_gen.empty_set_proc_param(sd.srv_, path.front()),
				sd.srv_,
				cyng::numeric_cast<std::uint8_t>(reader.get("nr"), 0u),
				cyng::value_cast<std::string>(reader.get("meter"), "")));
			break;
		case sml::CODE_DELETE_DEVICE:
			//
			//	remove a meter device from list of visible meters
			//
			messages.push_back(set_proc_parameter_delete_device(sml_gen.empty_set_proc_param(sd.srv_, path.front()),
				sd.srv_,
				cyng::numeric_cast<std::uint8_t>(reader.get("nr"), 0u),
				cyng::value_cast<std::string>(reader.get("meter"), "")));
			break;
		case sml::CODE_ROOT_SENSOR_PARAMS:
			messages.push_back(set_proc_parameter_sensor_params(sml_gen.empty_set_proc_param(sd.srv_, path.front()),
				sd.srv_,
				cyng::value_cast<std::string>(reader["data"].get("user"), ""),
				cyng::value_cast<std::string>(reader["data"].get("pwd"), ""),
				cyng::value_cast<std::string>(reader["data"].get("pubKey"), ""),
				cyng::value_cast<std::string>(reader["data"].get("aesKey"), "")));

			break;
		case sml::CODE_ROOT_BROKER:
			messages.push_back(set_proc_parameter_broker(sml_gen.empty_set_proc_param(sd.srv_, path.front()),
				sd.srv_,
				cyng::to_vector(reader.get("brokers"))));
			break;
		case sml::CODE_ROOT_SERIAL:
			//	ToDo: find the proper index with help of the cache
		{int i = 0; }
			break;

		default:
			messages.push_back(sml_gen.empty_set_proc_param(sd.srv_, path.front()));
			break;
		}

		//
		//	update - add a get-proc-param request to update cache
		//
		messages.push_back(sml_gen.get_proc_parameter(sd.srv_, path));

		//
		//	add close message
		//
		messages.push_back(sml_gen.public_close());

		//
		//	build complete
		//
		return proxy::make_data(std::move(messages)
			, std::move(cd)
			, std::move(sd)
			, path
			, cache_enabled);

	}

	cyng::tuple_t set_proc_parameter_hardware_port(cyng::tuple_t&& msg
		, cyng::buffer_t const& server_id
		, std::string name
		, std::uint8_t idx
		, cyng::tuple_t tpl)
	{
		sml::merge_msg(msg
			, { sml::OBIS_ROOT_SERIAL, sml::OBIS_SERIAL_NAME }
		, node::sml::make_value(name));

		for (auto const& obj : tpl) {

			auto const param = cyng::to_param(obj);

			if (boost::algorithm::equals(param.first, "databits")) {

				const auto databits = cyng::numeric_cast<std::uint32_t>(param.second, 9600u);
				sml::merge_msg(msg
					, { sml::OBIS_ROOT_SERIAL, make_obis(sml::OBIS_ROOT_SERIAL, idx), make_obis(sml::OBIS_SERIAL_DATABITS, idx) }
				, node::sml::make_value(databits));
			}
			else if (boost::algorithm::equals(param.first, "parity")) {
				sml::merge_msg(msg
					, { sml::OBIS_ROOT_SERIAL, make_obis(sml::OBIS_ROOT_SERIAL, idx), make_obis(sml::OBIS_SERIAL_PARITY, idx) }
				, node::sml::make_value(param.second));
			}
			else if (boost::algorithm::equals(param.first, "flowcontrol")) {
				sml::merge_msg(msg
					, { sml::OBIS_ROOT_SERIAL, make_obis(sml::OBIS_ROOT_SERIAL, idx), make_obis(sml::OBIS_SERIAL_FLOW_CONTROL, idx) }
				, node::sml::make_value(param.second));
			}
			else if (boost::algorithm::equals(param.first, "stopbits")) {
				sml::merge_msg(msg
					, { sml::OBIS_ROOT_SERIAL, make_obis(sml::OBIS_ROOT_SERIAL, idx), make_obis(sml::OBIS_SERIAL_STOPBITS, idx) }
				, node::sml::make_value(param.second));
			}
			else if (boost::algorithm::equals(param.first, "baudrate")) {
				sml::merge_msg(msg
					, { sml::OBIS_ROOT_SERIAL, make_obis(sml::OBIS_ROOT_SERIAL, idx), make_obis(sml::OBIS_SERIAL_SPEED, idx) }
				, node::sml::make_value(param.second));
			}
		}
		return msg;
	}

	proxy::data finalize_set_proc_parameter_request(
		proxy::cluster_data&& cd,
		proxy::sml_data&& sd,
		sml::obis root,				//	OBIS root code
		cyng::tuple_t const& params,
		bool cache_enabled)
	{
		auto const reader = cyng::make_reader(params);

		if (sml::OBIS_ACTIVATE_DEVICE == root) {
			//
			//	activate meter device
			//
			auto const nr = cyng::numeric_cast<std::uint8_t>(reader.get("nr"), 0u);
			auto const meter = cyng::value_cast<std::string>(reader.get("meter"), "");
			auto const r = cyng::parse_hex_string(meter);
			if (r.second) {
				return finalize_set_proc_parameter_request(
					std::move(cd),
					proxy::sml_data(sd, r.first),	//	substitute server id with meter id
					sml::obis_path_t({ root
						, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFB, nr)
						, sml::OBIS_SERVER_ID }),
					reader,
					cache_enabled);
			}
		}
		else if (sml::OBIS_DEACTIVATE_DEVICE == root) {
			//
			//	deactivate meter device
			//
			auto const nr = cyng::numeric_cast<std::uint8_t>(reader.get("nr"), 0u);
			auto const meter = cyng::value_cast<std::string>(reader.get("meter"), "");
			auto const r = cyng::parse_hex_string(meter);
			if (r.second) {
				return finalize_set_proc_parameter_request(
					std::move(cd),
					proxy::sml_data(sd, r.first),	//	substitute server id with meter id
					sml::obis_path_t({ root
						, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFC, nr)
						, sml::OBIS_SERVER_ID }),	
					reader,
					cache_enabled);
			}
		}
		else if (sml::OBIS_DELETE_DEVICE == root) {
			//
			//	remove a meter device from list of visible meters
			//
			auto const nr = cyng::numeric_cast<std::uint8_t>(reader.get("nr"), 0u);
			auto const meter = cyng::value_cast<std::string>(reader.get("meter"), "");
			auto const r = cyng::parse_hex_string(meter);
			if (r.second) {
				return finalize_set_proc_parameter_request(
					std::move(cd),
					proxy::sml_data(sd, r.first),	//	substitute server id with meter id
					sml::obis_path_t({ root
						, sml::make_obis(0x81, 0x81, 0x11, 0x06, 0xFD, nr)
						, sml::OBIS_SERVER_ID }),	
					reader,
					cache_enabled);
			}
		}
		else if (sml::OBIS_ROOT_SENSOR_PARAMS == root) {
			//
			//	meter ID is used as server ID
			//
			auto const meter = cyng::value_cast<std::string>(reader.get("meter"), "");
			auto const r = cyng::parse_hex_string(meter);
			if (r.second) {
				return finalize_set_proc_parameter_request(
					std::move(cd),
					proxy::sml_data(sd, r.first),	//	substitute server id with meter id
					sml::obis_path_t({ root }),		//	OBIS root code
					reader,
					cache_enabled);
			}
		}
		else if (sml::OBIS_ROOT_SERIAL == root) {

			node::sml::req_generator sml_gen(sd.name_, sd.pwd_);
			sml::messages_t messages{ sml_gen.public_open(get_mac(), sd.srv_) };

			for (auto const& obj : params) {
				auto const param = cyng::to_param(obj);
				auto const values = cyng::to_tuple(param.second);
				auto const dom = cyng::make_reader(values);
				std::uint8_t idx = cyng::numeric_cast<std::uint8_t>(dom.get("index"), 1u);
				BOOST_ASSERT(idx != 0);
				messages.push_back(set_proc_parameter_hardware_port(sml_gen.empty_set_proc_param(sd.srv_, root),
					sd.srv_,
					param.first,
					idx,
					values));
			}

			//
			//	update - add a get-proc-param request to update cache
			//
			messages.push_back(sml_gen.get_proc_parameter(sd.srv_, sml::obis_path_t({ root })));
			messages.push_back(sml_gen.public_close());

			return proxy::make_data(std::move(messages)
				, std::move(cd)
				, std::move(sd)
				, sml::obis_path_t({ root })
				, cache_enabled);
		}

		//
		//	generate messages
		//
		return finalize_set_proc_parameter_request(
			std::move(cd),
			std::move(sd),
			sml::obis_path_t({ root }),				//	OBIS root code
			reader,
			cache_enabled);

	}

	proxy::data finalize_get_profile_list_request_op_log(
		proxy::cluster_data&& cd,
		proxy::sml_data&& sd,
		sml::obis_path_t&& path,		//	OBIS path
		std::uint32_t hours,
		bool cache_enabled)
	{
		//
		//	SML request generator
		//
		node::sml::req_generator sml_gen(sd.name_, sd.pwd_);

		//
		//	generates multiple responses with the same transaction id
		//
		sml::messages_t messages{
			sml_gen.public_open(get_mac(), sd.srv_),
			sml_gen.get_profile_list(sd.srv_,
				std::chrono::system_clock::now() - std::chrono::hours(hours),
				std::chrono::system_clock::now(),
				path.front()),
			sml_gen.public_close()
		};

		BOOST_ASSERT(messages.size() == 3);

		return proxy::make_data(std::move(messages)
			, std::move(cd)
			, std::move(sd)
			, path
			, cache_enabled);

	}

	proxy::data finalize_get_profile_list_request(
		proxy::cluster_data&& cd,
		proxy::sml_data&& sd,
		sml::obis root,		//	OBIS root code
		cyng::tuple_t const& params,
		bool cache_enabled)
	{
		auto const reader = cyng::make_reader(params);

		if (sml::OBIS_CLASS_OP_LOG == root) {
			//
			//	get time range
			//
			auto const hours = cyng::numeric_cast<std::uint32_t>(reader.get("range"), 24u);
			return finalize_get_profile_list_request_op_log(
				std::move(cd),
				std::move(sd),	
				sml::obis_path_t({ root }),
				hours,
				cache_enabled);

		}

		//
		//	ToDo: implement
		//
		return proxy::make_data(sml::messages_t()
			, std::move(cd)
			, std::move(sd)
			, sml::obis_path_t({ root })
			, cache_enabled);
	}

	proxy::data finalize_get_list_request_current_data_record(
		proxy::cluster_data&& cd,
		proxy::sml_data&& sd,
		sml::obis_path_t&& path,		//	OBIS path
		cyng::tuple_reader const& reader,
		bool cache_enabled)
	{
		//
		//	SML request generator
		//
		node::sml::req_generator sml_gen(sd.name_, sd.pwd_);

		//
		//	initialize with open message
		//
		sml::messages_t messages{ 
			sml_gen.public_open(get_mac(), sd.srv_),
			sml_gen.get_list_last_data_record(get_mac().to_buffer(), sd.srv_),
			sml_gen.public_close()
		};

		//
		//	add close message
		//
		messages.push_back(sml_gen.public_close());

		return proxy::make_data(std::move(messages)
			, std::move(cd)
			, std::move(sd)
			, path
			, cache_enabled);
	}

	proxy::data finalize_get_list_request(
		proxy::cluster_data&& cd,
		proxy::sml_data&& sd,
		sml::obis root,		//	OBIS root code
		cyng::tuple_t const& params,
		bool cache_enabled)
	{
		auto const reader = cyng::make_reader(params);

		if (sml::OBIS_LIST_CURRENT_DATA_RECORD == root) {
			auto const meter = cyng::value_cast<std::string>(reader.get("meter"), "");
			auto const r = sml::parse_srv_id(meter);
			if (r.second) {

				return finalize_get_list_request_current_data_record(
					std::move(cd),
					proxy::sml_data(sd, r.first),	//	substitute server id with meter id
					sml::obis_path_t({ root }),
					reader,
					cache_enabled);

			}
		}
		//
		//	ToDo: implement
		//
		return proxy::make_data(sml::messages_t()
			, std::move(cd)
			, std::move(sd)
			, sml::obis_path_t({ root })
			, cache_enabled);
	}

	proxy::data make_proxy_data(boost::uuids::uuid tag,		//	[0] ident tag (target)
		boost::uuids::uuid source,	//	[1] source tag
		std::uint64_t seq,			//	[2] cluster seq
		boost::uuids::uuid origin,	//	[3] ws tag (origin)

		std::string const& channel,		//	[4] channel (SML message type)
		sml::obis code,				//	[5] OBIS root code
		cyng::vector_t const& gw,			//	[6] TGateway/TDevice PK
		cyng::tuple_t const& params,		//	[7] parameters

		cyng::buffer_t const& srv_id,		//	[8] server id
		std::string const& name,			//	[9] name
		std::string const& pwd,			//	[10] pwd
		bool cache_enabled			//	[11] use cache
	)
	{
		auto const msg_code = sml::messages::get_msg_code(channel);
		switch (msg_code)
		{
		case sml::message_e::GET_PROC_PARAMETER_REQUEST:
			return finalize_get_proc_parameter_request(
				proxy::cluster_data(tag, source, seq, origin, gw),
				proxy::sml_data(msg_code, srv_id, name, pwd),
				code,
				params,
				cache_enabled);
		case sml::message_e::SET_PROC_PARAMETER_REQUEST:
			return finalize_set_proc_parameter_request(
				proxy::cluster_data(tag, source, seq, origin, gw),
				proxy::sml_data(msg_code, srv_id, name, pwd),
				code,
				params,
				cache_enabled);
		case sml::message_e::GET_PROFILE_LIST_REQUEST:
			return finalize_get_profile_list_request(
				proxy::cluster_data(tag, source, seq, origin, gw),
				proxy::sml_data(msg_code, srv_id, name, pwd),
				code,
				params,
				cache_enabled);
		case sml::message_e::GET_LIST_REQUEST:
			return finalize_get_list_request(
				proxy::cluster_data(tag, source, seq, origin, gw),
				proxy::sml_data(msg_code, srv_id, name, pwd),
				code,
				params,
				cache_enabled);
			break;
		case sml::message_e::GET_PROFILE_PACK_REQUEST:
			//	not implemented
			break;
		default:
			break;
		}

		//
		//	error case
		//
		return proxy::make_data(sml::messages_t()
			, proxy::cluster_data(tag, source, seq, origin, gw)
			, proxy::sml_data(msg_code, srv_id, name, pwd)
			, sml::obis_path_t({ code })
			, cache_enabled);
	}

	namespace proxy
	{

		//
		//	cluster communication
		//
		cluster_data::cluster_data(
			boost::uuids::uuid tag,		//	[0] ident tag (target session)
			boost::uuids::uuid source,	//	[1] source tag
			std::uint64_t seq,			//	[2] cluster seq
			boost::uuids::uuid origin,	//	[3] ws tag (origin)
			cyng::vector_t gw)				//	[6] TGateway/Device PK
			: tag_(tag)			//	ident tag (target)
			, source_(source)	//	source tag
			, seq_(seq)			//	cluster seq
			, origin_(origin)	//	ws tag (origin)
			, gw_(gw)			//	TGateway/TDevice PK
		{}

		cluster_data cluster_data::copy() const
		{
			return *this;
		}

		//
		//	SML parameters
		//
		sml_data::sml_data(
			sml::message_e msg_code,	//	[4] SML message type
			cyng::buffer_t srv,		//	[8] server id
			std::string name,		//	[9] name
			std::string pwd			//	[10] pwd
		)
			: msg_code_(msg_code)	//	SML message type (explicit type conversion)
			, srv_(srv)			//	server id
			, name_(name)		//	name
			, pwd_(pwd)			//	pwd
		{}

		sml_data::sml_data(sml_data const& sd, cyng::buffer_t const& meter)
			: msg_code_(sd.msg_code_)	//	SML message type (explicit type conversion)
			, srv_(meter)			//	meter id
			, name_(sd.name_)		//	name
			, pwd_(sd.pwd_)			//	pwd
		{}

		sml_data sml_data::copy() const
		{
			return *this;
		}

		bool sml_data::is_msg_type(sml::message_e msg_code) const
		{
			return msg_code_ == msg_code;
		}

		data::data(sml::messages_t&& messages, reply_data&& reply)
			: messages_(std::move(messages))
			, reply_(std::move(reply))
		{}

		data make_data(sml::messages_t&& msgs, reply_data&& reply)
		{
			return data(std::move(msgs), std::move(reply));
		}

		data make_data(sml::messages_t&& msgs, 
			cluster_data&& cd,
			sml_data&& sd,
			sml::obis_path_t path,
			bool job)
		{
			return data(std::move(msgs), reply_data(std::move(cd), std::move(sd), path, job));
		}

	}

	cyng::mac48 get_mac()
	{
		auto const macs = cyng::sys::retrieve_mac48();

		return (macs.empty())
			? macs.front()
			: cyng::generate_random_mac48()
			;
	}


}


