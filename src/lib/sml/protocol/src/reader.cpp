#include <smf/sml/reader.h>
#include <smf/sml/readout.h>
#include <smf/obis/defs.h>
#include <smf/mbus/units.h>

#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>

#ifdef _DEBUG_SML
#include <iostream>
#endif

#include <boost/assert.hpp>

namespace smf {
	namespace sml {

		std::tuple<std::string, cyng::buffer_t, cyng::buffer_t, std::string, std::string, std::string, std::uint8_t>
			read_public_open_request(cyng::tuple_t msg) {
			BOOST_ASSERT(msg.size() == 7);
			if (msg.size() == 7) {

				//
				//	iterate over message
				//
				auto pos = msg.begin();

				//
				//	codepage "ISO 8859-15"
				//	mostly empty
				//
				auto const cp = to_string(*pos++);

				//
				//	clientId (MAC)
				//	Typically 7 bytes to identify gateway/MUC
				auto const client = cyng::to_buffer(*pos++);

				//
				//	reqFileId
				//
				auto const req = to_string(*pos++);

				//
				//	serverId
				//
				auto const server = cyng::to_buffer(*pos++);

				//
				//	username
				//
				auto const user = to_string(*pos++);

				//
				//	password
				//
				auto const pwd = to_string(*pos++);

				//
				//	sml-Version: default = 1
				//
				auto const version = read_numeric<std::uint8_t>(*pos++);

				return std::make_tuple(cp, client, server, req, user, pwd, version);

			}
			return std::make_tuple(std::string(), cyng::buffer_t(), cyng::buffer_t(), std::string(), std::string(), std::string(), 0);
		}

		std::tuple<std::string, cyng::buffer_t, cyng::buffer_t, std::string, cyng::object, std::uint8_t>
		read_public_open_response(cyng::tuple_t msg) {
			BOOST_ASSERT(msg.size() == 6);
			if (msg.size() == 6) {
				//
				//	iterate over message
				//
				auto pos = msg.begin();

				//	codepage "ISO 8859-15"
				auto const cp = to_string(*pos++);

				//
				//	clientId (MAC)
				//	Typically 7 bytes to identify gateway/MUC
				auto const client = cyng::to_buffer(*pos++);

				//
				//	reqFileId
				//
				auto const req = to_string(*pos++);

				//
				//	serverId
				//
				auto const server = cyng::to_buffer(*pos++);

				//
				//	refTime
				//
				auto const ref_time = *pos++;

				//	sml-Version: default = 1
				auto const version = read_numeric<std::uint8_t>(*pos++);

				return std::make_tuple(cp, client, server, req, ref_time, version);


			}
			return std::make_tuple(std::string(), cyng::buffer_t(), cyng::buffer_t(), std::string(), cyng::make_object(), 0);
		}

		cyng::object
		read_public_close_request(cyng::tuple_t msg) {
			BOOST_ASSERT(msg.size() == 1);
			if (msg.size() == 1) {
				//
				//	iterate over message
				//
				return *msg.begin();

			}
			return cyng::make_object();
		}

		cyng::object
		read_public_close_response(cyng::tuple_t msg) {
			BOOST_ASSERT(msg.size() == 1);
			if (msg.size() == 1) {
				//
				//	iterate over message
				//
				return *msg.begin();
			}
			return cyng::make_object();
		}


		std::tuple<cyng::buffer_t, cyng::buffer_t, cyng::obis, cyng::object, cyng::object, std::map<cyng::obis, cyng::param_map_t>> 
		read_get_list_response(cyng::tuple_t msg) {

#ifdef _DEBUG_SML
			std::cout << "> " << msg << std::endl;
#endif
			BOOST_ASSERT(msg.size() == 7);
			if (msg.size() == 7) {

				//
				//	iterate over message
				//
				auto pos = msg.begin();

				//
				//	client id
				//	mostly empty
				//
				auto const client = cyng::to_buffer(*pos++);


				//
				//	serverId - meter ID
				//
				auto const server = cyng::to_buffer(*pos++);

				//
				//	list name - optional
				//
				auto code = read_obis(*pos++);
				if (cyng::is_nil(code)) {
					code = OBIS_LIST_CURRENT_DATA_RECORD;	//	last data set (99 00 00 00 00 03)
				}

				//
				//	read act. sensor time - optional
				//
				auto const sensor_tp = read_time(*pos++);

				//
				//	valList
				//
				auto const tpl = cyng::container_cast<cyng::tuple_t>(*pos++);
				auto const r = read_sml_list(tpl.begin(), tpl.end());

				//
				//	SML_Signature - OPTIONAL
				//
				auto const sign = *pos++;

				//
				//	SML_Time - OPTIONAL 
				//
				auto const gw_tp = read_time(*pos++);

				return std::make_tuple(client, server, code, sensor_tp, gw_tp, r);

			}

			return std::make_tuple(cyng::buffer_t()
				, cyng::buffer_t()
				, cyng::obis()
				, cyng::object()
				, cyng::object()
				, std::map<cyng::obis
				, cyng::param_map_t>());

		}

		void read_get_proc_parameter_request(cyng::tuple_t msg) {
			BOOST_ASSERT(msg.size() == 5);
			if (msg.size() == 5) {

				//
				//	serverId
				//
				//auto const srv_id = read_server_id(*pos++);
				//boost::ignore_unused(srv_id);

				//
				//	username
				//
				//set_value("userName", read_string(*pos++));

				//
				//	password
				//
				//set_value("password", read_string(*pos++));

				//
				//	parameterTreePath == parameter address
				//	std::vector<obis>
				//
				//auto const path = read_param_tree_path(*pos++);
				//BOOST_ASSERT_MSG(!path.empty(), "no OBIS path");

				//
				//	attribute/constraints
				//
				//	*pos
				//return std::make_pair(path, *pos);


			}
		}

		void read_set_proc_parameter_request(cyng::tuple_t msg) {
			BOOST_ASSERT(msg.size() == 5);
			if (msg.size() == 5) {

				//
				//	serverId
				//
				//auto const srv_id = read_server_id(*pos++);
				//boost::ignore_unused(srv_id);

				//
				//	username
				//
				//set_value("userName", read_string(*pos++));

				//
				//	password
				//
				//set_value("password", read_string(*pos++));

				//
				//	parameterTreePath == parameter address
				//
				//auto const path = read_param_tree_path(*pos++);

				//
				//	each setProcParamReq has to send an attension message as response
				//

				//
				//	recursiv call to an parameter tree - similiar to read_param_tree()
				//	ToDo: use a generic implementation like in SML_GetProcParameter.Req
				//
				//auto p = read_set_proc_parameter_request_tree(path, 0, *pos++);
				//prg.insert(prg.end(), p.begin(), p.end());
				//
				//	parameterTree
				//
				//auto const tpl = cyng::to_tuple(*pos++);
				//if (tpl.size() != 3) return std::make_pair(path, cyng::param_t{});	//return cyng::generate_invoke("log.msg.error", "SML Tree", tpl.size());

				//auto const param = read_param_tree(0u, tpl.begin(), tpl.end());

				//return std::make_pair(path, param);
			}
		}

		std::map<cyng::obis, cyng::param_map_t> read_sml_list(cyng::tuple_t::const_iterator pos
			, cyng::tuple_t::const_iterator end)
		{
			std::map<cyng::obis, cyng::param_map_t>	r;

			//
			//	list of tuples (period entry)
			//
			while (pos != end) {
				cyng::tuple_t tpl;
				tpl = cyng::value_cast(*pos++, tpl);
				auto const [code, pm] = read_list_entry(tpl.begin(), tpl.end());
				BOOST_ASSERT(r.find(code) == r.end());	//	no duplicates allowed
				r.emplace(code, pm);
			}

			return r;
		}

		std::pair<cyng::obis, cyng::param_map_t> read_list_entry(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end)
		{
			std::size_t count = std::distance(pos, end);
			BOOST_ASSERT_MSG(count == 7, "List Entry");
			if (count != 7) return std::make_pair(cyng::obis(), cyng::param_map_factory());

			//
			//	object name
			//
			auto const code = read_obis(*pos++);

			//
			//	status - optional
			//
			auto const status = *pos++;

			//
			//	valTime - optional
			//
			auto const val_time = read_time(*pos++);

			//
			//	unit (see sml_unit_enum) - optional
			//
			auto const unit = read_numeric<std::uint8_t>(*pos++);
			auto const unit_name = smf::mbus::get_unit_name(smf::mbus::to_unit(unit));

			//
			//	scaler - optional
			//
			auto const scaler = read_numeric<std::int8_t>(*pos++);

			//
			//	value - note value with OBIS code
			//
			auto raw = *pos++;
			auto val = read_value(code, scaler, unit, raw);
			return std::make_pair(code, cyng::param_map_factory
				("unit", unit)
				("unit-name", unit_name)
				("scaler", scaler)
				("value", val)
				("raw", raw)
				("valTime", val_time)
				("status", status)
				("signature", *pos)
			);
		}


	}
}
