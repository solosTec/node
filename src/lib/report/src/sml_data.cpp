
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

    // cyng::object sml_data::restore() const { return smf::restore(code_, reading_, scaler_); }

    // cyng::object restore(std::uint16_t code, std::string const &val, std::int8_t scaler) {
    //     switch (code) {
    //     case cyng::TC_BUFFER: return restore_buffer(val);
    //     case cyng::TC_INT32:
    //     case cyng::TC_INT64: return restore_int(val, code, scaler);
    //     default: break;
    //     }
    //     return cyng::restore(val, code);
    // }

    // cyng::object restore_buffer(std::string const &val) {
    //     if (val.size() % 2 != 0) {
    //         //  skip
    //         return cyng::make_object(cyng::make_buffer());
    //     }
    //     return cyng::restore(val, cyng::TC_BUFFER);
    // }

    // cyng::object restore_int(std::string val, std::uint16_t code, std::int8_t scaler) {
    //     if (scaler != 0) {
    //         //  remove "."
    //         val.erase(std::remove(val.begin(), val.end(), '.'), val.end());
    //     }
    //     return cyng::restore(val, code);
    // }

    lpex_customer::lpex_customer(std::string id, std::string mc, std::string name, std::string unique_name)
        : id_(id)
        , mc_(mc)
        , name_(name)
        , unique_name_(unique_name) {}

} // namespace smf
