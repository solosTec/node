/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_REPORT_SML_DATAH
#define SMF_REPORT_SML_DATAH

#include <smf/mbus/units.h>

#include <cyng/obj/intrinsics/obis.h>
#include <cyng/obj/object.h>

namespace smf {

    struct sml_data {
        sml_data(std::uint16_t code, std::int8_t scaler, std::uint8_t unit, std::string reading, std::uint32_t status);

        [[nodiscard]] cyng::object restore() const;

        std::uint16_t code_;
        std::int8_t scaler_;
        mbus::unit unit_;
        std::string reading_;
        std::uint32_t status_;
    };

    // cyng::object restore(std::uint16_t code, std::string const &val, std::int8_t);
    // cyng::object restore_buffer(std::string const &val);
    // cyng::object restore_int(std::string val, std::uint16_t code, std::int8_t);

    struct lpex_customer {
        lpex_customer(std::string, std::string, std::string, std::string);
        std::string id_;          //  customer id - Kundennummer: (example "11013951")
        std::string mc_;          //  metering code: (example "CH1015201234500000000000000032418")
        std::string name_;        //  Kundenname
        std::string unique_name_; //  unique name
    };

} // namespace smf

#endif
