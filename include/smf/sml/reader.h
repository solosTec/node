/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_READER_H
#define SMF_SML_READER_H

#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/obis.h>
#include <cyng/obj/object.h>

#include <cstdint>

namespace smf {
    namespace sml {

        /**
         * define data type to hold a SML list
         */
        using sml_list_t = std::map<cyng::obis, cyng::param_map_t>;

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
         */
        void read_get_proc_parameter_request(cyng::tuple_t msg);

        /** @brief SET_PROC_PARAMETER_REQUEST (1536)
         */
        void read_set_proc_parameter_request(cyng::tuple_t msg);

        /** @brief GET_PROC_PARAMETER_RESPONSE (1281)
         */
        [[nodiscard]] std::pair<cyng::obis_path_t, cyng::param_t> read_get_proc_parameter_response(cyng::tuple_t msg);

        /** @brief message_e::GET_PROFILE_LIST_RESPONSE (1025)
         */
        [[nodiscard]] std::tuple<cyng::buffer_t, cyng::object, std::uint32_t, std::uint32_t, cyng::obis_path_t, sml_list_t>
        read_get_profile_list_response(cyng::tuple_t msg);

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
         * read a data record
         */
        [[nodiscard]] std::pair<cyng::obis, cyng::object>
        read_param_tree(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);

    } // namespace sml
} // namespace smf
#endif
