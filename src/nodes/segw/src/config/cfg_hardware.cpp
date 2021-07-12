/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_hardware.h>
#include <smf/obis/defs.h>

#include <cyng/parse/buffer.h>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

namespace smf {

    cfg_hardware::cfg_hardware(cfg &c)
        : cfg_(c) {}

    namespace {
        std::string serial_path() { return cyng::to_path(cfg::sep, cfg_hardware::root, "serial"); }
        std::string model_path() { return cyng::to_path(cfg::sep, cfg_hardware::root, "model"); }
        std::string manufacturer_path() { return cyng::to_path(cfg::sep, cfg_hardware::root, "manufacturer"); }
        std::string class_path() { return cyng::to_path(cfg::sep, cfg_hardware::root, "class"); }

        std::string adapter_manufacturer_path() {
            return cyng::to_path(cfg::sep, cfg_hardware::root, "adapter", OBIS_W_MBUS_ADAPTER_MANUFACTURER);
        }
        std::string adapter_id() { return cyng::to_path(cfg::sep, cfg_hardware::root, "adapter", OBIS_W_MBUS_ADAPTER_ID); }
        std::string adapter_firmware_version() {
            return cyng::to_path(cfg::sep, cfg_hardware::root, "adapter", OBIS_W_MBUS_FIRMWARE);
        }
        std::string adapter_hardware_version() {
            return cyng::to_path(cfg::sep, cfg_hardware::root, "adapter", OBIS_W_MBUS_HARDWARE);
        }

    } // namespace

    std::string cfg_hardware::get_model() const { return cfg_.get_value(model_path(), "model"); }

    std::uint32_t cfg_hardware::get_serial_number() const { return cfg_.get_value(serial_path(), 10000000u); }

    std::string cfg_hardware::get_manufacturer() const { return cfg_.get_value(manufacturer_path(), "solosTec"); }

    cyng::obis cfg_hardware ::get_device_class() const {
        auto const str = cfg_.get_value(class_path(), "8181C78202FF");
        auto const buffer = cyng::hex_to_buffer(str);
        return (buffer.size() == cyng::obis::size())
                   ? cyng::make_obis(buffer.at(0), buffer.at(1), buffer.at(2), buffer.at(3), buffer.at(4), buffer.at(5))
                   : cyng::make_obis(0x81, 0x81, 0xC7, 0x82, 0x02, 0xFF);
    }

    cyng::buffer_t cfg_hardware::get_adapter_id() const {
        auto const str = cfg_.get_value(adapter_id(), "A815174589030131");
        return cyng::hex_to_buffer(str);
    }

    std::string cfg_hardware::get_adapter_manufacturer() const {
        return cfg_.get_value(adapter_manufacturer_path(), "RC1180-MBUS3");
    }

    std::string cfg_hardware::get_adapter_firmware_version() const { return cfg_.get_value(adapter_firmware_version(), "3.08"); }
    std::string cfg_hardware::get_adapter_hardware_version() const { return cfg_.get_value(adapter_hardware_version(), "2.00"); }

} // namespace smf
