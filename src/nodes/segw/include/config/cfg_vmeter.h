/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_CONFIG_VMETER_H
#define SMF_SEGW_CONFIG_VMETER_H

#include <cfg.h>
#include <config/cfg_lmn.h>

namespace smf {

    class cfg_vmeter {
      public:
        cfg_vmeter(cfg &, lmn_type);
        cfg_vmeter(cfg_vmeter &) = default;

        /**
         * numerical value of the specified LMN enum type
         */
        constexpr std::uint8_t get_index() const { return static_cast<std::uint8_t>(get_type()); }
        constexpr lmn_type get_type() const { return type_; }
        std::string get_path_id() const;

        bool is_enabled() const;
        cyng::buffer_t get_server() const;
        std::string get_protocol() const;
        std::chrono::seconds get_interval() const;

        constexpr static char root[] = "virtual-meter";

      private:
        cfg &cfg_;
        lmn_type const type_;
    };

} // namespace smf

#endif
