
#include <smf/report/sml_data.h>

#include <cyng/db/storage.h>

#include <iostream>

namespace smf {

    sml_data::sml_data(std::uint16_t code, std::int8_t scaler, std::uint8_t unit, std::string reading, std::uint32_t status)
        : code_(code)
        , scaler_(scaler)
        , unit_(mbus::to_unit(unit))
        , reading_(reading)
        , status_(status) {}

    lpex_customer::lpex_customer(std::string id, std::string mc, std::string name, std::string unique_name)
        : id_(id)
        , mc_(mc)
        , name_(name)
        , unique_name_(unique_name) {}

} // namespace smf
