/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_REPORT_SML_DATAH
#define SMF_REPORT_SML_DATAH

#include <smf/mbus/server_id.h>
#include <smf/mbus/units.h>

#include <cyng/obj/intrinsics/obis.h>
#include <cyng/obj/object.h>

#include <map>

namespace smf {

    struct sml_data {
        sml_data(std::uint16_t code, std::int8_t scaler, std::uint8_t unit, std::string reading, std::uint32_t status);

        std::uint16_t code_;
        std::int8_t scaler_;
        mbus::unit unit_;
        std::string reading_;
        std::uint32_t status_;
    };

    /**
     * calculate delta
     */
    std::string operator-(sml_data const &, sml_data const &);

    /**
     * data structure to hold a complete profile readout
     */
    namespace data {

        //
        //  slot -> data
        //
        using readout_t = std::map<std::int64_t, sml_data>;

        //
        //  register -> slot -> data
        //
        using values_t = std::map<cyng::obis, readout_t>;

        //
        //  data set of the full period
        //  meter -> register -> slot -> data
        //
        using data_set_t = std::map<smf::srv_id_t, values_t>;
    } // namespace data

    struct lpex_customer {
        lpex_customer(std::string, std::string, std::string, std::string);
        std::string id_;          //  customer id - Kundennummer: (example "11013951")
        std::string mc_;          //  metering code: (example "CH1015201234500000000000000032418")
        std::string name_;        //  Kundenname
        std::string unique_name_; //  unique name
    };

} // namespace smf

#endif
