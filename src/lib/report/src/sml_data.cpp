
#include <smf/report/sml_data.h>

#include <smf/sml/readout.h>

#include <cyng/db/storage.h>

#include <iostream>

namespace smf {

    sml_data::sml_data(std::uint16_t code, std::int8_t scaler, std::uint8_t unit, std::string reading, std::uint32_t status)
        : code_(code)
        , scaler_(scaler)
        , unit_(mbus::to_unit(unit))
        , reading_(reading)
        , status_(status) {}

    std::string operator-(sml_data const &first, sml_data const &second) {
        //  work with strings only without lost of precision
        if (first.scaler_ == second.scaler_) {

            //
            //  calculate difference with integers
            //
            auto const delta =
                smf::scale_value_reverse(first.reading_, first.scaler_) - smf::scale_value_reverse(second.reading_, second.scaler_);

            //
            //  convert back to string
            //
            return smf::scale_value(delta, second.scaler_);
        }
        auto const v0 = std::stod(first.reading_);
        auto const v1 = std::stod(second.reading_);
        std::stringstream ss;
        auto const delta = v1 - v0;
        if (delta < 1) {
            ss << std::setprecision(1) << std::fixed << delta;
        } else {
            ss << std::setprecision(std::abs(first.scaler_)) << std::fixed << delta;
        }
        return ss.str();
    }

    lpex_customer::lpex_customer(std::string id, std::string mc, std::string name, std::string unique_name)
        : id_(id)
        , mc_(mc)
        , name_(name)
        , unique_name_(unique_name) {}

} // namespace smf
