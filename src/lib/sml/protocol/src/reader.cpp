#include <smf/sml/reader.h>

#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/sml/readout.h>

#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>

#ifdef _DEBUG_SML
#include <cyng/io/ostream.h>
#include <iostream>
#endif
#ifdef _DEBUG
#include <cyng/io/ostream.h>
#endif

#include <chrono>
#include <memory>

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
            return std::make_tuple(
                std::string(), cyng::buffer_t(), cyng::buffer_t(), std::string(), std::string(), std::string(), 0);
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

        cyng::object read_public_close_request(cyng::tuple_t msg) {
            BOOST_ASSERT(msg.size() == 1);
            if (msg.size() == 1) {
                //
                //	iterate over message
                //  global signature
                //
                return *msg.begin();
            }
            return cyng::make_object();
        }

        cyng::object read_public_close_response(cyng::tuple_t msg) {
            BOOST_ASSERT(msg.size() == 1);
            if (msg.size() == 1) {
                //
                //	iterate over message
                //
                return *msg.begin();
            }
            return cyng::make_object();
        }

        std::tuple<cyng::buffer_t, cyng::buffer_t, std::string, std::string, cyng::obis> read_get_list_request(cyng::tuple_t msg) {
            BOOST_ASSERT(msg.size() == 5);
            if (msg.size() == 5) {
                //
                //	iterate over message
                //
                auto pos = msg.begin();

                //
                //	client id - MAC address from caller
                //
                auto const client = cyng::to_buffer(*pos++);

                //
                //	serverId - meter ID
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
                //	OBIS code
                //
                auto const code = read_obis(*pos++);

                return {client, server, user, pwd, code};
            }
            return {{}, {}, {}, {}, {}};
        }

        std::tuple<cyng::buffer_t, cyng::buffer_t, cyng::obis, cyng::object, cyng::object, sml_list_t>
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
                    code = OBIS_LIST_CURRENT_DATA_RECORD; //	last data set (99 00 00 00 00 03)
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

            return std::make_tuple(cyng::buffer_t(), cyng::buffer_t(), cyng::obis(), cyng::object(), cyng::object(), sml_list_t());
        }

        std::tuple<cyng::buffer_t, std::string, std::string, cyng::obis_path_t, cyng::object>
        read_get_proc_parameter_request(cyng::tuple_t msg) {
            BOOST_ASSERT(msg.size() == 5);
            if (msg.size() == 5) {

                //
                //	iterate over message
                //
                auto pos = msg.begin();

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
                //	parameterTreePath == parameter address
                //	std::vector<obis>
                //
                auto const path = read_param_tree_path(*pos++);

                //
                //	attribute/constraints
                //
                //	*pos
                return std::make_tuple(server, user, pwd, path, *pos);
            }
            return std::make_tuple(cyng::buffer_t(), std::string(), std::string(), cyng::obis_path_t(), cyng::make_object());
        }

        std::tuple<cyng::buffer_t, std::string, std::string, cyng::obis_path_t, cyng::obis, cyng::attr_t, cyng::tuple_t>
        read_set_proc_parameter_request(cyng::tuple_t msg) {
            BOOST_ASSERT(msg.size() == 5);
            if (msg.size() == 5) {

                //
                //	iterate over message
                //
                auto pos = msg.begin();

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
                //	parameterTreePath == parameter address
                //	std::vector<obis>
                //
                auto const path = read_param_tree_path(*pos++);

                //
                //	each setProcParamReq has to send an attension message as response
                //

                //
                //	parameterTree
                //
                auto const tpl = cyng::container_cast<cyng::tuple_t>(*pos++);
                BOOST_ASSERT(tpl.size() == 3);

                auto [code, attr, list] = read_param_tree(tpl.begin(), tpl.end());

                return std::make_tuple(server, user, pwd, path, code, attr, list);
            }
            return std::make_tuple(
                cyng::buffer_t{}, std::string{}, std::string{}, cyng::obis_path_t{}, cyng::obis{}, cyng::attr_t{}, cyng::tuple_t{});
        }

        std::tuple<cyng::buffer_t, cyng::obis_path_t, cyng::obis, cyng::attr_t, cyng::tuple_t>
        read_get_proc_parameter_response(cyng::tuple_t msg) {
            BOOST_ASSERT(msg.size() == 3);
            if (msg.size() == 3) {

                //
                //	iterate over message
                //
                auto pos = msg.begin();

                //
                //	serverId
                //
                auto const server = cyng::to_buffer(*pos++);

                //
                //	parameterTreePath (OBIS)
                //
                auto const path = read_param_tree_path(*pos++);

                //
                //	parameterTree
                //
                auto const tpl = cyng::container_cast<cyng::tuple_t>(*pos++);
                if (tpl.size() != 3) {
                    return {server, path, {}, {}, {}};
                }

                //
                //  * OBIS
                //  * parameterValue
                //  * child_List
                //
                auto [c, p, l] = read_param_tree(tpl.begin(), tpl.end());
                return {server, path, c, p, l};
                //  list of parameter trees (read_param_tree())
            }
            return {{}, {}, {}, {}, {}};
        }

        std::tuple<
            cyng::buffer_t,
            std::string,
            std::string,
            bool,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point,
            cyng::obis_path_t,
            cyng::object,
            cyng::object>
        read_get_profile_list_request(cyng::tuple_t msg) {
            BOOST_ASSERT(msg.size() == 9);
            if (msg.size() == 9) {
                //
                //	iterate over message
                //
                auto pos = msg.begin();

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

                auto const has_raw_data = cyng::value_cast(*pos++, false);

                //
                //	beginTime
                //
                auto const now = std::chrono::system_clock::now();
                auto const begin_tp = cyng::value_cast(read_time(*pos++), now - std::chrono::hours(24));

                //
                //	endTime
                //
                auto const end_tp = cyng::value_cast(read_time(*pos++), now);

                //
                //	parameterTreePath == parameter address
                //	std::vector<obis>
                //
                auto const path = read_param_tree_path(*pos++);

                auto const obj_list = *pos++;
                auto const das_details = *pos++;

                return std::make_tuple(server, user, pwd, has_raw_data, begin_tp, end_tp, path, obj_list, das_details);
            }

            return {{}, {}, {}, {}, {}, {}, {}, {}, {}};
        }

        std::tuple<cyng::buffer_t, cyng::obis_path_t, cyng::object, std::chrono::seconds, cyng::object, std::uint32_t, sml_list_t>
        read_get_profile_list_response(cyng::tuple_t msg) {
            BOOST_ASSERT(msg.size() == 9);
            if (msg.size() == 9) {

                //
                //	iterate over message
                //
                auto pos = msg.begin();

                //
                //	serverId
                //
                auto const server = cyng::to_buffer(*pos++);

                //
                //	actTime
                //
                auto const sensor_tp = read_time(*pos++);

                //
                //	regPeriod (u32 => seconds)
                //
                auto const reg_period = read_numeric<std::uint32_t>(*pos++);

                //
                //	parameterTreePath (OBIS)
                //
                auto const path = read_param_tree_path(*pos++);

                //
                //	valTime
                //
                auto const val_tp = read_time(*pos++);

                //
                //	M-bus status
                //
                auto const status = read_numeric<std::uint32_t>(*pos++);

                //	period-List (1)
                //	first we make the TSMLGetProfileListResponse complete, then
                //	the we store the entries in TSMLPeriodEntry.
                auto const tpl = cyng::container_cast<cyng::tuple_t>(*pos++);
                auto const r = read_period_list(tpl.begin(), tpl.end());

                //	rawdata
                auto const raw_data = *pos++;

                //	periodSignature
                auto const signature = *pos;

                return std::make_tuple(server, path, sensor_tp, std::chrono::seconds(reg_period), val_tp, status, r);
            }
            return std::make_tuple(
                cyng::buffer_t(),
                cyng::obis_path_t(),
                cyng::make_object(),
                std::chrono::seconds{},
                cyng::make_object(),
                std::uint32_t{},
                sml_list_t{});
        }

        cyng::obis read_attention_response(cyng::tuple_t msg) {
            BOOST_ASSERT(msg.size() == 4);
            if (msg.size() == 4) {

                //
                //	iterate over message
                //
                auto pos = msg.begin();

                //
                //	serverId
                //
                auto const server = cyng::to_buffer(*pos++);

                //
                //  attention code
                //
                auto code = read_obis(*pos++);

                //
                //  attention message (optional)
                //
                auto const msg = *pos++;

                //
                //	parameterTree (optional)
                //
                auto const tpl = cyng::container_cast<cyng::tuple_t>(*pos++);
                // auto const r = (tpl.size() == 3) ? read_param_tree(tpl.begin(), tpl.end()) : std::pair<cyng::obis,
                // cyng::object>{};

                return code;
            }
            return cyng::obis();
        }

        sml_list_t read_sml_list(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end) {
            sml_list_t r;

            //
            //	list of tuples (period entry)
            //
            while (pos != end) {
                auto const tpl = cyng::container_cast<cyng::tuple_t>(*pos++);
                BOOST_ASSERT_MSG(tpl.size() == 7, "List Entry");
                if (tpl.size() == 7) {
                    auto const [code, pm] = read_list_entry(tpl.begin(), tpl.end());
                    if (r.find(code) == r.end()) {
                        r.emplace(code, pm);
                    }
                    // else {
                    //    BOOST_ASSERT_MSG(false, "no duplicates allowed"); //	no duplicates allowed
                    //}
                }
            }

            return r;
        }

        std::pair<cyng::obis, cyng::param_map_t>
        read_list_entry(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end) {
            std::size_t count = std::distance(pos, end);
            BOOST_ASSERT_MSG(count == 7, "List Entry");
            if (count != 7)
                return std::make_pair(cyng::obis(), cyng::param_map_factory());

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
            auto const unit_name = smf::mbus::get_name(smf::mbus::to_unit(unit));

            //
            //	scaler - optional
            //
            auto const scaler = read_numeric<std::int8_t>(*pos++);

            //
            //	value - note value with OBIS code
            //
            auto raw = *pos;
            auto val = read_value(code, scaler, raw);
            return std::make_pair(
                code,
                cyng::param_map_factory("unit", unit)("unit-name", unit_name)("scaler", scaler)("value", val)("raw", raw)(
                    "valTime", val_time)("status", status));
        }

        sml_list_t read_period_list(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end) {

            sml_list_t r;

            //
            //	list of tuples (period entry)
            //
            while (pos != end) {
                auto const tpl = cyng::container_cast<cyng::tuple_t>(*pos++);
                BOOST_ASSERT_MSG(tpl.size() == 5, "Period Entry");
                if (tpl.size() == 5) {
                    auto const [code, pm] = read_period_entry(tpl.begin(), tpl.end());
                    BOOST_ASSERT(r.find(code) == r.end()); //	no duplicates allowed
                    r.emplace(code, pm);
                }
            }

            return r;
        }

        std::pair<cyng::obis, cyng::param_map_t>
        read_period_entry(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end) {
            std::size_t count = std::distance(pos, end);
            BOOST_ASSERT_MSG(count == 5, "Period Entry");
            if (count != 5)
                return std::make_pair(cyng::obis(), cyng::param_map_factory());

            //
            //	object name
            //
            auto const code = read_obis(*pos++);

            //
            //	unit (see sml_unit_enum)
            //
            auto const u = read_numeric<std::uint8_t>(*pos++);
            auto const unit_e = smf::mbus::to_unit(u);
            auto const unit_name = get_unit_name(code, unit_e);

            //
            //	scaler
            //
            auto const scaler = read_numeric<std::int8_t>(*pos++);

            //
            //	value
            //
            auto raw = *pos++;
            auto val = read_value(code, scaler, raw);

            //
            //	valueSignature
            //
            auto const signature = *pos;

            //
            //	make a parameter
            //
            return std::make_pair(
                code,
                cyng::param_map_factory("unit", u)("unit-name", unit_name)("scaler", scaler)("value", val)("raw", raw)(
                    "value", val));
        }

        std::tuple<cyng::obis, cyng::attr_t, cyng::tuple_t>
        read_param_tree(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end) {
            std::size_t count = std::distance(pos, end);
            BOOST_ASSERT_MSG(count == 3, "SML Tree");
            if (count != 3) {
                return {{}, {}, {}};
            }

            //
            //	object name
            //
            auto const code = read_obis(*pos++);

            //
            //	parameterValue SML_ProcParValue OPTIONAL,
            //
            auto const attr = read_parameter(code, *pos++);

            //
            //	3. child_List List_of_SML_Tree OPTIONAL
            //
            cyng::tuple_t child_list;
            auto const tpl = cyng::container_cast<cyng::tuple_t>(*pos);
            for (auto const &child : tpl) {

                //
                //  build child list
                //
                auto tree = cyng::container_cast<cyng::tuple_t>(child);
                if (tree.size() == 3) {

                    //  recursion
                    auto [c, p, l] = read_param_tree(tree.begin(), tree.end());
                    child_list.push_back(cyng::make_object(cyng::make_tuple(c, p, l)));
                }
            }

            return {code, attr, child_list};
        }

    } // namespace sml

    std::string get_unit_name(cyng::obis code, mbus::unit u) {
        switch (u) {
        case mbus::unit::UNDEFINED_:
        case mbus::unit::OTHER_UNIT:
        case mbus::unit::COUNT: return obis::get_name(code);
        default: break;
        }
        return mbus::get_name(u);
    }

} // namespace smf
