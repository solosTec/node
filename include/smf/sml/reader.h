/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_READER_H
#define SMF_SML_READER_H

#include <smf/mbus/units.h>

#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/date.h>
#include <cyng/obj/intrinsics/obis.h>
#include <cyng/obj/object.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace smf {

    std::string get_unit_name(cyng::obis, mbus::unit);

    namespace sml {

        /**
         * define data type to hold a SML list
         */
        using sml_list_t = std::map<cyng::obis, cyng::param_map_t>;

        /**
         * @return codepage, client ID (05+MAC), reqFileId, server ID, username, password, SML version
         */
        [[nodiscard]] std::tuple<std::string, cyng::buffer_t, cyng::buffer_t, std::string, std::string, std::string, std::uint8_t>
        read_public_open_request(cyng::tuple_t msg);

        /**
         * @return codepage, client ID (05+MAC), reqFileId, server ID, refTime, SML version
         */
        [[nodiscard]] std::tuple<std::string, cyng::buffer_t, cyng::buffer_t, std::string, cyng::object, std::uint8_t>
        read_public_open_response(cyng::tuple_t msg);

        /**
         * @return global signature
         */
        cyng::object read_public_close_request(cyng::tuple_t msg);

        /**
         * @return global signature
         */
        cyng::object read_public_close_response(cyng::tuple_t msg);

        /** @brief GET_LIST_REQUEST (1792)
         *
         * SML push data
         *
         * @return tuple with client id, server id, list name (obis code), sensor time, gateway time and a map
         * with all readouts.
         */
        [[nodiscard]] std::tuple<cyng::buffer_t, cyng::buffer_t, std::string, std::string, cyng::obis>
        read_get_list_request(cyng::tuple_t msg);

        /** @brief GET_LIST_RESPONSE (1793)
         *
         * SML push data
         *
         * @return tuple with client id, server id, list name (obis code), sensor time, gateway time and a map
         * with all readouts.
         */
        [[nodiscard]] std::tuple<cyng::buffer_t, cyng::buffer_t, cyng::obis, cyng::object, cyng::object, sml_list_t>
        read_get_list_response(cyng::tuple_t msg);

        /** @brief GET_PROC_PARAMETER_REQUEST (1280)
         *
         * @return server id, user, password, parameter tree path, attribute
         */
        [[nodiscard]] std::tuple<cyng::buffer_t, std::string, std::string, cyng::obis_path_t, cyng::object>
        read_get_proc_parameter_request(cyng::tuple_t msg);

        /** @brief SET_PROC_PARAMETER_REQUEST (1536)
         *  each setProcParamReq has to send an attension message as response
         */
        [[nodiscard]] std::
            tuple<cyng::buffer_t, std::string, std::string, cyng::obis_path_t, cyng::obis, cyng::attr_t, cyng::tuple_t>
            read_set_proc_parameter_request(cyng::tuple_t msg);

        /** @brief GET_PROC_PARAMETER_RESPONSE (1281)
         *
         * @return server id, obis path, code (root), attribute value (mostly empty), child list with data
         */
        [[nodiscard]] std::tuple<cyng::buffer_t, cyng::obis_path_t, cyng::obis, cyng::attr_t, cyng::tuple_t>
        read_get_proc_parameter_response(cyng::tuple_t msg);

        /** @brief message_e::GET_PROFILE_LIST_REQUEST (1024)
         *  @return server id, username, password, withRawData, beginTime, endTime, parameterTreePath, objList, dasDetails
         */
        [[nodiscard]] std::tuple<
            cyng::buffer_t,
            std::string,
            std::string,
            bool,
            cyng::date,
            cyng::date,
            cyng::obis_path_t,
            cyng::object,
            cyng::object>
        read_get_profile_list_request(cyng::tuple_t msg);

        /** @brief message_e::GET_PROFILE_LIST_RESPONSE (1025)
         *  @return server id, tree path, act time, register period, val time, M-Bus state, period list
         */
        [[nodiscard]] std::
            tuple<cyng::buffer_t, cyng::obis_path_t, cyng::object, std::chrono::seconds, cyng::object, std::uint32_t, sml_list_t>
            read_get_profile_list_response(cyng::tuple_t msg);

        /**
         * @return attention code
         */
        [[nodiscard]] cyng::obis read_attention_response(cyng::tuple_t msg);

        [[nodiscard]] sml_list_t read_sml_list(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);

        /**
         * Reads a set of 7 values:
         * unit, scaler, raw, valTime, status and signature
         */
        [[nodiscard]] std::pair<cyng::obis, cyng::param_map_t>
        read_list_entry(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);

        /**
         * Read a list of period entries
         */
        [[nodiscard]] sml_list_t read_period_list(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);

        /**
         * Reads a set of 5 values:
         * obis code, unit, scaler, raw, value and signature
         */
        [[nodiscard]] std::pair<cyng::obis, cyng::param_map_t>
        read_period_entry(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);

        /**
         * read a SML tree.
         * A SML tree consists of a parameter name (OBIS), a value and and an child list. In most cases the value and the child list
         * are exclusive.
         */
        [[nodiscard]] std::tuple<cyng::obis, cyng::attr_t, cyng::tuple_t>
        read_param_tree(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);

    } // namespace sml
} // namespace smf
#endif
