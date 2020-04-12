/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/protocol/readout.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/sml/ip_io.h>
#include <smf/mbus/defs.h>
#include <smf/sml/scaler.h>

#include <cyng/io/io_buffer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/io/serializer.h>
#include <cyng/factory.h>
#include <cyng/buffer_cast.h>
#include <cyng/set_cast.h>
#include <cyng/numeric_cast.hpp>

#include <boost/core/ignore_unused.hpp>

namespace node
{
	namespace sml
	{
		readout::readout()
			: trx_()
			, server_id_()
			, client_id_()
			, values_()
		{}

		void readout::reset()
		{
			trx_.clear();
			server_id_.clear();
			client_id_.clear();
			values_.clear();
		}

		readout& readout::set_trx(cyng::buffer_t const& buffer)
		{
			trx_ = cyng::io::to_ascii(buffer);
			return *this;
		}

		readout& readout::set_pk(boost::uuids::uuid pk)
		{
			return set_value("pk", cyng::make_object(pk));
		}

		readout& readout::set_value(std::string const& name, cyng::object obj)
		{
			values_.emplace(name, obj);
			return *this;
		}

		readout& readout::set_value(cyng::param_t const& param)
		{
			values_[param.first] = param.second;
			return *this;
		}

		readout& readout::set_value(cyng::param_map_t&& params)
		{
			//	preserve the elements in values_
			//values_.insert(params.begin(), params.end());

			//	overwrite existing values
			for (auto const& value : params) {
				values_[value.first] = value.second;
			}
			return *this;
		}

		readout& readout::set_value(obis code, cyng::object obj)
		{
			values_[code.to_str()] = obj;
			return *this;
		}

		readout& readout::set_value(obis code, std::string name, cyng::object obj)
		{
			BOOST_ASSERT_MSG(!name.empty(), "name for map entry is empty");
			if (values_.find(name) == values_.end()) {

				//
				//	new element
				//
				auto map = cyng::param_map_factory(code.to_str(), obj)();
				values_.emplace(name, map);
			}
			else {

				//
				//	more elements
				//
				auto p = cyng::object_cast<cyng::param_map_t>(values_.at(name));
				if (p != nullptr) {
					const_cast<cyng::param_map_t*>(p)->emplace(code.to_str(), obj);
				}
			}
			return *this;
		}

		cyng::object readout::get_value(std::string const& name) const
		{
			auto pos = values_.find(name);
			return (pos != values_.end())
				? pos->second
				: cyng::make_object()
				;
		}

		cyng::object readout::get_value(obis code) const
		{
			return get_value(code.to_str());
		}

		std::string readout::get_string(obis code) const
		{
			auto const obj = get_value(code);
			auto const buffer = cyng::to_buffer(obj);
			return std::string(buffer.begin(), buffer.end());
		}

		std::string readout::read_server_id(cyng::object obj)
		{
			server_id_ = cyng::value_cast(obj, server_id_);
			return from_server_id(server_id_);
		}

		std::string readout::read_client_id(cyng::object obj)
		{
			client_id_ = cyng::value_cast(obj, client_id_);
			return from_server_id(client_id_);
		}

		std::pair<sml_message, cyng::tuple_t> readout::read_choice(cyng::object obj)
		{
			auto const choice = cyng::to_tuple(obj);
			BOOST_ASSERT_MSG(choice.size() == 2, "CHOICE");
			if (choice.size() == 2)
			{
				set_value("code", choice.front());
				auto const code = static_cast<sml_message>(cyng::value_cast<std::uint16_t>(choice.front(), 0));
				return std::make_pair(code, cyng::to_tuple(choice.back()));
			}
			return std::make_pair(sml_message::UNKNOWN, cyng::tuple_t{});
		}

		std::pair<sml_message, cyng::tuple_t> readout::read_msg(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			auto const count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "SML message");
			if (count != 5)	return std::make_pair(sml_message::UNKNOWN, cyng::tuple_t{});

			//
			//	(1) - transaction id
			//	This is typically a printable sequence of 6 up to 19 ASCII values
			//
			auto const buffer = cyng::to_buffer(*pos++);
			set_trx(buffer);

			//
			//	(2) groupNo
			//
			set_value("groupNo", *pos++);

			//
			//	(3) abortOnError
			//
			set_value("abortOnError", *pos++);

			//
			//	(4/5) CHOICE - msg type
			//
			auto const choice = read_choice(*pos++);

			//
			//	(6) CRC16
			//
			set_value("crc16", *pos);

			return choice;
		}

		bool readout::read_public_open_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			auto const count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 7, "Public Open Request");
			if (count != 7)	return false;	// cyng::generate_invoke("log.msg.error", "SML Public Open Request", count);

			//	codepage "ISO 8859-15"
			set_value("codepage", *pos++);

			//
			//	clientId (MAC)
			//	Typically 7 bytes to identify gateway/MUC
			auto const client_id = read_client_id(*pos++);
			boost::ignore_unused(client_id);

			//
			//	reqFileId
			//
			set_value("reqFileId", read_string(*pos++));

			//
			//	serverId
			//
			auto const srv_id = read_server_id(*pos++);
			boost::ignore_unused(srv_id);

			//
			//	username
			//
			set_value("userName", read_string(*pos++));

			//
			//	password
			//
			set_value("password", read_string(*pos++));

			//
			//	sml-Version: default = 1
			//
			set_value("SMLVersion", *pos++);

			return true;
		}

		bool readout::read_public_open_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			auto const count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 6, "Public Open Response");
			if (count != 6)	return false; // cyng::generate_invoke("log.msg.error", "SML Public Open Response", count);

			//	codepage "ISO 8859-15"
			set_value("codepage", *pos++);

			//
			//	clientId (MAC)
			//	Typically 7 bytes to identify gateway/MUC
			auto const client_id = read_client_id(*pos++);
			boost::ignore_unused(client_id);

			//
			//	reqFileId
			//
			set_value("reqFileId", read_string(*pos++));

			//
			//	serverId
			//
			auto const srv_id = read_server_id(*pos++);
			boost::ignore_unused(srv_id);

			//
			//	refTime
			//
			set_value("refTime", *pos++);

			//	sml-Version: default = 1
			set_value("SMLVersion", *pos++);

			return true;
		}

		bool readout::read_public_close_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			auto const count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 1, "Public Close Request");
			if (count != 1)	return false; // cyng::generate_invoke("log.msg.error", "SML Public Close Request", count);

			set_value("globalSignature", *pos++);
			return true;
		}

		bool readout::read_public_close_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			auto const count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 1, "Public Close Response");
			if (count != 1)	return false;	// cyng::generate_invoke("log.msg.error", "SML Public Close Response", count);

			set_value("globalSignature", *pos++);
			return true;
		}

		std::pair<obis_path, cyng::param_t> readout::read_get_proc_parameter_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			auto const count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "Get Proc Parameter Response");
			if (count != 3)	return std::make_pair(obis_path{}, cyng::param_t{});

			//
			//	serverId
			//
			auto const srv_id = read_server_id(*pos++);
			boost::ignore_unused(srv_id);

			//
			//	parameterTreePath (OBIS)
			//
			auto const path = read_param_tree_path(*pos++);

			//
			//	parameterTree
			//
			auto const tpl = cyng::to_tuple(*pos++);
			if (tpl.size() != 3)	return std::make_pair(path, cyng::param_t{});

			//
			//	this is a parameter tree
			//
			auto const param = read_param_tree(0u, tpl.begin(), tpl.end());

			return std::make_pair(path, param);
		}

		std::pair<obis_path, cyng::object> readout::read_get_proc_parameter_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "Get Proc Parameter Request");
			if (count != 5)	return std::make_pair(obis_path{}, cyng::make_object());

			//
			//	serverId
			//
			auto const srv_id = read_server_id(*pos++);
			boost::ignore_unused(srv_id);

			//
			//	username
			//
			set_value("userName", read_string(*pos++));

			//
			//	password
			//
			set_value("password", read_string(*pos++));

			//
			//	parameterTreePath == parameter address
			//	std::vector<obis>
			//
			auto const path = read_param_tree_path(*pos++);
			BOOST_ASSERT_MSG(!path.empty(), "no OBIS path");

			//
			//	attribute/constraints
			//
			//	*pos
			return std::make_pair(path, *pos);

		}

		std::pair<obis_path, cyng::param_map_t> readout::read_get_profile_list_response(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 9, "Get Profile List Response");
			if (count != 9)	return std::make_pair(obis_path{}, cyng::param_map_t{});

			//
			//	serverId
			//
			auto const srv_id = read_server_id(*pos++);
			boost::ignore_unused(srv_id);

			//
			//	actTime
			//
			set_value("actTime", read_time(*pos++));

			//
			//	regPeriod
			//
			set_value("regPeriod", *pos++);

			//
			//	parameterTreePath (OBIS)
			//
			auto const path = read_param_tree_path(*pos++);

			//
			//	valTime
			//
			set_value("valTime", read_time(*pos++));

			//
			//	M-bus status
			//
			set_value("status", *pos++);

			//	period-List (1)
			//	first we make the TSMLGetProfileListResponse complete, then 
			//	the we store the entries in TSMLPeriodEntry.
			auto const tpl = cyng::to_tuple(*pos++);
			auto const params = read_period_list(tpl.begin(), tpl.end());

			//	rawdata
			set_value("rawData", *pos++);

			//	periodSignature
			set_value("signature", *pos++);

			return std::make_pair(path, params);

		}

		obis_path readout::read_get_profile_list_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 9, "Get Profile List Request");
			if (count != 9)	return obis_path{};

			//
			//	serverId
			//	Typically 7 bytes to identify gateway/MUC
			//
			auto const srv_id = read_server_id(*pos++);
			boost::ignore_unused(srv_id);

			//
			//	username
			//
			set_value("userName", read_string(*pos++));

			//
			//	password
			//
			set_value("password", read_string(*pos++));

			//
			//	rawdata (typically not set)
			//
			set_value("rawData", *pos++);

			//
			//	start/end time
			//
			set_value("beginTime", read_time(*pos++));
			set_value("endTime", read_time(*pos++));

			//	parameterTreePath (OBIS)
			//
			auto const path = read_param_tree_path(*pos++);
			BOOST_ASSERT(path.size() == 1);

			//
			//	object list
			//
			auto obj_list = *pos++;
			boost::ignore_unused(obj_list);

			//
			//	dasDetails
			//
			auto const das_details = *pos++;
			boost::ignore_unused(das_details);

			return path;
		}

		std::pair<obis_path, cyng::param_t> readout::read_set_proc_parameter_request(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			auto const count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "Set Proc Parameter Request");
			if (count != 5) return std::make_pair(obis_path{}, cyng::param_t{});; //cyng::generate_invoke("log.msg.error", "SML Set Proc Parameter Request", count);

			//
			//	serverId
			//
			auto const srv_id = read_server_id(*pos++);
			boost::ignore_unused(srv_id);

			//
			//	username
			//
			set_value("userName", read_string(*pos++));

			//
			//	password
			//
			set_value("password", read_string(*pos++));

			//
			//	parameterTreePath == parameter address
			//
			auto const path = read_param_tree_path(*pos++);

			//
			//	each setProcParamReq has to send an attension message as response
			//
			//auto prg = cyng::generate_invoke("sml.attention.init"
			//	, ro.trx_
			//	, ro.server_id_
			//	, OBIS_ATTENTION_OK.to_buffer()
			//	, "Set Proc Parameter Request");

			//
			//	recursiv call to an parameter tree - similiar to read_param_tree()
			//	ToDo: use a generic implementation like in SML_GetProcParameter.Req
			//
			//auto p = read_set_proc_parameter_request_tree(path, 0, *pos++);
			//prg.insert(prg.end(), p.begin(), p.end());
			//
			//	parameterTree
			//
			auto const tpl = cyng::to_tuple(*pos++);
			if (tpl.size() != 3) return std::make_pair(path, cyng::param_t{});	//return cyng::generate_invoke("log.msg.error", "SML Tree", tpl.size());

			auto const param = read_param_tree(0u, tpl.begin(), tpl.end());

			return std::make_pair(path, param);
			//prg << cyng::unwinder(cyng::generate_invoke("sml.set.proc.parameter.request"
			//	, ro.trx_
			//	, to_hex(path, ' ')	//	string
			//	, ro.server_id_
			//	, ro.get_value("userName")
			//	, ro.get_value("password")
			//	, param));

			//
			//	send prepared attention message
			//
			//prg << cyng::unwinder(cyng::generate_invoke("sml.attention.send", ro.trx_));

			//return prg;
		}

		//
		//	helper
		//
		cyng::object read_string(cyng::object obj)
		{
			cyng::buffer_t const buffer = cyng::to_buffer(obj);
			return cyng::make_object(cyng::io::to_ascii(buffer));
		}

		std::int8_t read_scaler(cyng::object obj)
		{
			return cyng::numeric_cast<std::int8_t>(obj, 0);
		}

		std::uint8_t read_unit(cyng::object obj)
		{
			return cyng::value_cast<std::uint8_t>(obj, 0);
		}

		obis_path read_param_tree_path(cyng::object obj)
		{
			std::vector<obis> result;

			cyng::tuple_t const tpl = cyng::to_tuple(obj);

			std::transform(tpl.begin(), tpl.end(), std::back_inserter(result), [](cyng::object obj) -> obis {
				return read_obis(obj);
				});

			return result;
		}

		obis read_obis(cyng::object obj)
		{
			auto const tmp = cyng::to_buffer(obj);

			return (tmp.empty())
				? obis()
				: obis(tmp)
				;
		}

		cyng::param_t read_param_tree(std::size_t depth, cyng::object obj)
		{
			cyng::tuple_t const tpl = cyng::to_tuple(obj);

			return (tpl.size() == 3)
				? read_param_tree(depth, tpl.begin(), tpl.end())
				: cyng::param_t{}
			;
		}

		cyng::param_t read_param_tree(std::size_t depth
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 3, "SML Tree");
			if (count != 3) return cyng::param_t{};

			//
			//	1. parameterName Octet String,
			//
			auto const code = read_obis(*pos++);

			//
			//	2. parameterValue SML_ProcParValue OPTIONAL,
			//
			auto const attr = read_parameter(*pos++);
			cyng::tuple_t const tpl = cyng::to_tuple(*pos++);
			if (tpl.empty()) {
				return (attr.first != PROC_PAR_UNDEF)
					? customize_value(code, attr.second)
					: cyng::param_factory(code.to_str(), cyng::make_object())
					;
			}

			//
			//	collect all optional values in a parameter map
			//
			cyng::param_map_t params;

			//
			//	if attr contains some valuable information 
			//	put it into the result set - but flattended.
			//	Note thate there is posible collision with
			//	OBIS codes in the child list. Until now
			//	no collisions are known.
			//
			if (attr.first != PROC_PAR_UNDEF) {
				params.emplace(customize_value(code, attr.second));
			}

			//
			//	3. child_List List_of_SML_Tree OPTIONAL
			//
			//BOOST_ASSERT(!tpl.empty()); -> empty trees are possible e.g. 00, 80, 80, 00, 04, FF (certificates)

			if ((OBIS_ROOT_VISIBLE_DEVICES == code) || (OBIS_ROOT_ACTIVE_DEVICES == code)) {

				//
				//	build a vector of devices
				//
				return collect_devices(code, tpl);
			}
			else if (OBIS_IF_1107_METER_LIST == code) {
				//
				//	build a vector of devices
				//	81 81 C7 93 09 FF
				//
				return collect_iec_devices(tpl);
			}

			for (auto const child : tpl)
			{
				BOOST_ASSERT(child.get_class().tag() == cyng::TC_TUPLE);
				cyng::tuple_t const tree = cyng::to_tuple(child);
				BOOST_ASSERT_MSG(tree.size() == 3, "SML Tree");

				auto const param = read_param_tree(depth + 1, tree.begin(), tree.end());

				//
				//	collect all optional values in a parameter map.
				//	No duplicates allowed
				//
				BOOST_ASSERT(params.find(param.first) == params.end());
				params.emplace(param.first, param.second);

			}

			//
			//	detect OBIS codes with index
			//
			if (code.is_matching(0x81, 0x49, 0x0D, 0x07, 0x00).second) {
				//	IP-T parameters
				params.emplace(cyng::param_factory("idx", code.get_storage()));
			}
			else if (code.is_matching(0x00, 0x80, 0x80, 0x11, 0x01).second) {
				//	Actuators:
				params.emplace(cyng::param_factory("idx", code.get_storage()));
			}
			else if (code.is_matching(0x81, 0x81, 0xC7, 0x82, 0x07).second) {
				//	firmware
				params.emplace(cyng::param_factory("idx", code.get_storage()));
			}
			else if (OBIS_DATA_PUBLIC_KEY == code
				|| OBIS_ROOT_SENSOR_BITMASK == code
				|| OBIS_DATA_USER_NAME == code
				|| OBIS_DATA_USER_PWD == code) {
				return cyng::param_factory(code.to_str(), "");
			}

			return cyng::param_factory(code.to_str(), params);
		}

		cyng::attr_t read_parameter(cyng::object obj)
		{
			cyng::tuple_t const tpl = cyng::to_tuple(obj);
			if (tpl.size() == 2)
			{
				const auto type = cyng::value_cast<std::uint8_t>(tpl.front(), 0);
				switch (type)
				{
				case PROC_PAR_VALUE:		return cyng::attr_t(type, tpl.back());
				case PROC_PAR_PERIODENTRY:	return cyng::attr_t(type, tpl.back());
				case PROC_PAR_TUPELENTRY:	return cyng::attr_t(type, tpl.back());
				case PROC_PAR_TIME:			return cyng::attr_t(type, read_time(tpl.back()));
				default:
					break;
				}
			}
			return cyng::attr_t(PROC_PAR_UNDEF, cyng::make_object());
		}

		cyng::object read_time(cyng::object obj)
		{
			cyng::tuple_t const choice = cyng::to_tuple(obj);
			if (choice.empty())	return cyng::make_object();

			BOOST_ASSERT_MSG(choice.size() == 2, "TIME");
			if (choice.size() == 2)
			{
				auto code = cyng::value_cast<std::uint8_t>(choice.front(), 0);
				switch (code)
				{
				case TIME_TIMESTAMP:
				{
					const std::uint32_t sec = cyng::value_cast<std::uint32_t>(choice.back(), 0);
					return cyng::make_time_point(sec);
				}
				break;
				case TIME_SECINDEX:
					return choice.back();
				default:
					break;
				}
			}
			return cyng::make_object();
		}

		cyng::param_t customize_value(obis code, cyng::object obj)
		{
			if (OBIS_CODE(81, 49, 17, 07, 00, 01) == code
				|| OBIS_CODE(81, 49, 17, 07, 00, 02) == code) {
				return cyng::param_factory(code.to_str(), ip_address_to_str(obj));
			}
			else if (code.is_matching(0x81, 0x49, 0x1A, 0x07, 0x00).second
				|| OBIS_W_MBUS_MODE_S == code
				|| OBIS_W_MBUS_MODE_T == code) {
				return cyng::param_factory(code.to_str(), cyng::numeric_cast<std::uint16_t>(obj, 0u));
			}
			else if (code.is_matching(0x81, 0x49, 0x19, 0x07, 0x00).second) {
				return cyng::param_factory(code.to_str(), cyng::numeric_cast<std::uint16_t>(obj, 0u));
			}
			else if (code.is_matching(0x81, 0x49, 0x63, 0x3C, 0x01).second
				|| code.is_matching(0x81, 0x49, 0x63, 0x3C, 0x02).second
				|| OBIS_W_MBUS_ADAPTER_MANUFACTURER == code
				|| OBIS_W_MBUS_FIRMWARE == code
				|| OBIS_W_MBUS_HARDWARE == code
				|| OBIS_DATA_MANUFACTURER == code
				|| OBIS_DEVICE_KERNEL == code
				|| OBIS_VERSION == code
				|| OBIS_FILE_NAME == code
				|| OBIS_IF_1107_METER_ID == code
				|| OBIS_IF_1107_ADDRESS == code
				|| OBIS_IF_1107_P1 == code
				|| OBIS_IF_1107_W5 == code
				|| OBIS_PUSH_TARGET == code
				|| code.is_matching(0x81, 0x81, 0xC7, 0x82, 0x0A).second
				|| OBIS_ACCESS_USER_NAME == code) {
				//	buffer to string
				cyng::buffer_t const buffer = cyng::to_buffer(obj);
				return cyng::param_factory(code.to_str(), std::string(buffer.begin(), buffer.end()));
			}
			else if (OBIS_CODE(81, 49, 17, 07, 00, 00) == code) {
				return cyng::param_factory(code.to_str(), to_ip_address_v4(obj));
			}
			else if (OBIS_DEVICE_CLASS == code
				|| OBIS_DATA_AES_KEY == code) {

				if (obj.is_null()) {
					return cyng::param_factory(code.to_str(), "");
				}
			}
			else if (OBIS_CURRENT_UTC == code) {
				return cyng::param_factory(code.to_str(), std::chrono::system_clock::now());
			}
			return cyng::param_t(code.to_str(), obj);
		}

		cyng::param_t collect_devices(obis code, cyng::tuple_t const& tpl)
		{
			cyng::param_map_t data;
			for (auto const& outer_list : tpl) {

				cyng::tuple_t const devices = cyng::to_tuple(outer_list);
				BOOST_ASSERT(devices.size() == 3);	//	SML tree
				if (devices.size() == 3) {

					auto pos = devices.begin();

					//
					//	1. parameterName Octet String,
					//
					auto const sub_code = read_obis(*pos++);

					//
					//	2. parameterValue SML_ProcParValue OPTIONAL,
					//
					auto const attr = read_parameter(*pos++);
					boost::ignore_unused(attr);

					//
					//	3. child_List List_of_SML_Tree OPTIONAL
					//

					cyng::tuple_t const inner_list = cyng::to_tuple(*pos);

					for (auto const& device : inner_list) {
						auto const param = read_param_tree(0, device);

						//
						//	ToDo: add device type (node::sml::get_srv_type(meter))
						//	manufacturer
						//	if (type == node::sml::SRV_MBUS_WIRED || type == node::sml::SRV_MBUS_RADIO) {
						//		auto const code = node::sml::get_manufacturer_code(meter);
						//		maker = node::sml::decode(code);
						//	}
						//	and meter id (short)
						//

						cyng::param_map_t values = cyng::to_param_map(param.second);

						//
						//	get/set number from OBIS code
						//
						auto const dev_code = to_obis(param.first);
						values.emplace("nr", cyng::make_object(dev_code.get_number()));

						//
						//	* 81 81 C7 82 04 FF: server ID
						//	* 81 81 C7 82 02 FF: device class (mostly 2D 2D 2D)
						//	* 01 00 00 09 0B 00: timestamp
						//
						auto const pos_srv = values.find(OBIS_SERVER_ID.to_str());
						if (pos_srv != values.end()) {

							//
							//	get server ID
							//
							BOOST_ASSERT(pos_srv->second.get_class().tag() == cyng::TC_BUFFER);
							cyng::buffer_t const meter = cyng::to_buffer(pos_srv->second);
							values.emplace("meter", cyng::make_object(meter));	//	binary format

							//
							//	get server type
							//
							std::uint32_t const srv_type = get_srv_type(meter);
							values.emplace("type", cyng::make_object(srv_type));

							//
							//	get manufacturer
							//
							switch (srv_type) {
							case SRV_MBUS_WIRED:
							case SRV_MBUS_RADIO:
							case SRV_W_MBUS:
							{
								auto const code = get_manufacturer_code(meter);
								auto const maker = decode(code);
								values.emplace("maker", cyng::make_object(maker));
							}
							break;
							default:
								values.emplace("maker", cyng::make_object(""));
								break;
							}

							//
							//	get meter ID (4 bytes)
							//
							auto const serial = get_serial(meter);
							values.emplace("serial", cyng::make_object(serial));

							//
							//	update meter ident
							//
							std::string s = from_server_id(meter);
							pos_srv->second = cyng::make_object(s);

						}

						data.emplace(param.first, cyng::make_object(values));
					}
				}
			}
			return cyng::param_factory(code.to_str(), data);
		}

		cyng::param_t collect_iec_devices(cyng::tuple_t const& tpl)
		{
			cyng::vector_t vec;
			for (auto const& outer_list : tpl) {

				cyng::tuple_t const devices = cyng::to_tuple(outer_list);
				BOOST_ASSERT(devices.size() == 3);	//	SML tree
				if (devices.size() == 3) {

					auto pos = devices.begin();

					//
					//	1. parameterName Octet String,
					//
					auto const sub_code = read_obis(*pos++);

					//
					//	2. parameterValue SML_ProcParValue OPTIONAL,
					//
					auto const attr = read_parameter(*pos++);
					boost::ignore_unused(attr);

					//
					//	3. child_List List_of_SML_Tree OPTIONAL
					//	all IEC devices
					//

					cyng::tuple_t const device = cyng::to_tuple(*pos);

					cyng::param_map_t values;
					for (auto const& data : device) {

						//
						//	read device data
						//
						auto const param = read_param_tree(0, data);
						values.insert(param);

					}
					values.emplace("nr", cyng::make_object(sub_code.get_storage()));
					vec.push_back(cyng::make_object(values));
				}
			}
			return cyng::param_factory(OBIS_IF_1107_METER_LIST.to_str(), vec);
		}

		cyng::param_map_t read_period_list(cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			cyng::param_map_t params;

			//
			//	list of tuples (period entry)
			//
			std::size_t counter{ 0 };
			while (pos != end)
			{
				cyng::tuple_t tpl;
				tpl = cyng::value_cast(*pos++, tpl);
				auto const param = read_period_entry(counter, tpl.begin(), tpl.end());
				params.insert(param);
				++counter;
			}
			return params;
		}

		cyng::param_t read_period_entry(std::size_t index
			, cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 5, "Period Entry");
			if (count != 5) return cyng::param_factory("error", "read_period_entry");

			//
			//	object name
			//
			obis const code = read_obis(*pos++);

			//
			//	unit (see sml_unit_enum)
			//
			auto const unit = read_unit(*pos++);

			//
			//	scaler
			//
			const auto scaler = read_scaler(*pos++);

			//
			//	value
			//
			auto val = read_value(code, scaler, unit, *pos++);

			//
			//	valueSignature
			//
			//vec.push_back(*pos);

			//if (OBIS_PUSH_TARGET == code) {

			//}

			//
			//	make a parameter
			//
			return cyng::param_factory(code.to_str(), val);
		}

		cyng::object read_value(obis code, std::int8_t scaler, std::uint8_t unit, cyng::object obj)
		{
			//
			//	write value
			//
			if (OBIS_DATA_MANUFACTURER == code
				|| OBIS_PUSH_TARGET == code
				|| OBIS_DATA_PUSH_DETAILS == code)
			{
				return read_string(obj);
			}
			else if (OBIS_CURRENT_UTC == code)
			{
				if (obj.get_class().tag() == cyng::TC_TUPLE)
				{
					return read_time(obj);
				}
				else
				{
					auto const tm = cyng::value_cast<std::uint32_t>(obj, 0);
					auto const tp = std::chrono::system_clock::from_time_t(tm);
					return cyng::make_object(tp);
				}
			}
			else if (OBIS_ACT_SENSOR_TIME == code)
			{
				auto const tm = cyng::value_cast<std::uint32_t>(obj, 0);
				auto const tp = std::chrono::system_clock::from_time_t(tm);
				return cyng::make_object(tp);
			}
			else if (OBIS_SERIAL_NR == code)
			{
				cyng::buffer_t const buffer = cyng::to_buffer(obj);
				auto const serial_nr = cyng::io::to_hex(buffer);
				return cyng::make_object(serial_nr);
			}
			else if (OBIS_SERIAL_NR_SECOND == code)
			{
				cyng::buffer_t const buffer = cyng::to_buffer(obj);
				auto const serial_nr = cyng::io::to_hex(buffer);
				return cyng::make_object(serial_nr);
			}
			else if (OBIS_MBUS_STATE == code) {
				auto const state = cyng::numeric_cast<std::int32_t>(obj, 0);
				return cyng::make_object(state);
			}
			else if (OBIS_PEER_ADDRESS == code) {
				//	OBIS-T-Kennzahl der Ereignisquelle
				cyng::buffer_t const buffer = cyng::to_buffer(obj);
				return (buffer.size() == 6)
					? cyng::make_object(obis(buffer).to_str())
					: cyng::make_object(buffer)
					;
			}
			else if (OBIS_CLASS_EVENT == code) {
				auto const event = cyng::numeric_cast<std::uint32_t>(obj, 0);
				return cyng::make_object(event);
			}

			if (scaler != 0)
			{
				switch (obj.get_class().tag())
				{
				case cyng::TC_INT64:
				{
					std::int64_t const value = cyng::value_cast<std::int64_t>(obj, 0);
					auto const str = scale_value(value, scaler);
					return cyng::make_object(str);
				}
				break;
				case cyng::TC_INT32:
				{
					std::int32_t const value = cyng::value_cast<std::int32_t>(obj, 0);
					auto const str = scale_value(value, scaler);
					return cyng::make_object(str);
				}
				break;
				case cyng::TC_UINT64:
				{
					std::uint64_t const value = cyng::value_cast<std::uint64_t>(obj, 0);
					auto const str = scale_value(value, scaler);
					return cyng::make_object(str);
				}
				break;
				case cyng::TC_UINT32:
				{
					std::uint32_t const value = cyng::value_cast<std::uint32_t>(obj, 0);
					auto const str = scale_value(value, scaler);
					return cyng::make_object(str);
				}
				break;
				default:
					break;
				}
			}
			return  obj;
		}


	}	//	sml
}

