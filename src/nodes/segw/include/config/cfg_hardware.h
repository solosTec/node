/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_HARDWARE_H
#define SMF_SEGW_CONFIG_HARDWARE_H

#include <cfg.h>

namespace smf {

    class cfg_hardware {
      public:
        cfg_hardware(cfg &);
        cfg_hardware(cfg_hardware &) = default;

        std::string get_model() const;
        std::uint32_t get_serial_number() const;
        std::string get_manufacturer() const;
        cyng::obis get_device_class() const;

        /**
         * as defined in EN 13757-3/4
         */
        cyng::buffer_t get_adapter_id() const;
        std::string get_adapter_manufacturer() const;
        std::string get_adapter_firmware_version() const;
        std::string get_adapter_hardware_version() const;

        constexpr static char root[] = "hw";

      private:
        cfg &cfg_;
    };

} // namespace smf

#endif
