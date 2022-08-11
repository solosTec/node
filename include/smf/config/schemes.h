/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_CONFIG_SCHEMES_H
#define SMF_CONFIG_SCHEMES_H

#include <cyng/store/meta.h>

#include <boost/asio/ip/address.hpp>
#include <boost/uuid/name_generator.hpp>

namespace smf {
    namespace config {

        /**
         * device/TDevice
         */
        cyng::meta_store get_store_device();
        cyng::meta_sql get_table_device();

        /**
         * meter/TMeter
         */
        cyng::meta_store get_store_meter();
        cyng::meta_sql get_table_meter();

        /**
         * temporary table withh full info of all meters.
         * This includes data from IEC and wM-Bus meters
         */
        cyng::meta_store get_store_meter_full();

        /**
         * meterIEC/TMeterIEC
         */
        cyng::meta_store get_store_meterIEC();
        cyng::meta_sql get_table_meterIEC();

        /**
         * meterwMBus/TMeterwMBus
         */
        cyng::meta_store get_store_meterwMBus();
        cyng::meta_sql get_table_meterwMBus();

        /**
         * gateway/TGateway
         */
        cyng::meta_store get_store_gateway();
        cyng::meta_sql get_table_gateway();

        /**
         * loRaDevice/TLoRaDevice
         */
        cyng::meta_store get_store_lora();
        cyng::meta_sql get_table_lora();

        /**
         * guiUser/TGuiUser
         */
        cyng::meta_store get_store_gui_user();
        cyng::meta_sql get_table_gui_user();

        /**
         * location/TLocation
         */
        cyng::meta_store get_store_location();
        cyng::meta_sql get_table_location();

        /**
         * device configuration
         */
        cyng::meta_store get_store_cfg_set_meta();
        cyng::meta_sql get_table_cfg_set_meta();

        //
        //	store only schemes
        // -----------------------------------------------------------------+
        //

        /**
         * sessions
         */
        cyng::meta_store get_store_session();

        /**
         * all current connections
         */
        cyng::meta_store get_store_connection();

        /**
         * push targets
         */
        cyng::meta_store get_store_target();

        /**
         * push channels
         */
        cyng::meta_store get_store_channel();

        /**
         * cluster/TCluster
         */
        cyng::meta_store get_store_cluster();

        /**
         * super simple config table
         */
        cyng::meta_store get_config();

        /**
         * similar to "config" table but with additional
         * type info to reconstruct the original data type.
         */
        cyng::meta_sql get_table_config();

        /**
         * Same as get_table_config() but allows to specify
         * a table name other than "TConfig".
         */
        cyng::meta_sql get_table_config(std::string name);

        /**
         * system messages
         */
        cyng::meta_store get_store_sys_msg();

        /**
         * IEC gateway status
         */
        cyng::meta_store get_store_gwIEC();

        /**
         * wMBus gateway status
         */
        cyng::meta_store get_store_gwwMBus();

        /**
         * device configuration
         */
        cyng::meta_sql get_table_cfg_set();

        //
        //	uplinks
        //
        cyng::meta_store get_store_uplink_lora();
        cyng::meta_store get_store_uplink_iec();
        cyng::meta_store get_store_uplink_wmbus();

        /**
         * container to store meta data of in-memory tables
         */
        using store_map = std::map<std::string, cyng::meta_store>;

        /**
         * container to store meta data of SQL tables
         */
        using sql_map = std::map<std::string, cyng::meta_sql>;

        class dependend_key {
          public:
            dependend_key();

            static std::string build_name(std::string host, std::uint16_t port);
            cyng::key_t generate(std::string host, std::uint16_t port);
            cyng::key_t operator()(std::string host, std::uint16_t port);

            cyng::key_t generate(boost::asio::ip::address addr);
            cyng::key_t operator()(boost::asio::ip::address addr);

          private:
            /**
             * generate depended keys for "gwIEC"
             */
            boost::uuids::name_generator_sha1 uuid_gen_;
        };

        /**
         * @return true if the specified table name is one of the defined
         * in-memory tables in this module.
         */
        bool is_known_store_name(std::string);

        /**
         * UUID namespace to derive a tag from a meter name
         */
        static constexpr boost::uuids::uuid device_name{
            {0xd5, 0x88, 0x4b, 0xce, 0x9c, 0x2b, 0x40, 0xbf, 0x9f, 0xc0, 0x5c, 0x0e, 0x77, 0xf7, 0x08, 0xc3}};

        //
        //	readout data shared between "store" and "report" application
        // -----------------------------------------------------------------+
        //
        cyng::meta_store get_store_sml_readout();
        cyng::meta_store get_store_sml_readout_data();

        cyng::meta_sql get_table_sml_readout();
        cyng::meta_sql get_table_sml_readout_data();

        cyng::meta_store get_store_iec_readout();
        cyng::meta_store get_store_iec_readout_data();

        cyng::meta_sql get_table_iec_readout();
        cyng::meta_sql get_table_iec_readout_data();

        cyng::meta_store get_store_customer();
        cyng::meta_sql get_table_customer();
        cyng::meta_store get_store_meter_lpex();
        cyng::meta_sql get_table_meter_lpex();

    } // namespace config
} // namespace smf

#endif
