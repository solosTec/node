#include <smf/mbus/server_id.h>
#include <smf/sml/generator.h>
#include <smf/sml/msg.h>

#include <cyng/obj/util.hpp>

#include <algorithm>

#include <boost/assert.hpp>

namespace smf {
    namespace sml {
        namespace detail {

            trx::trx()
                : gen_(cyng::crypto::make_rnd_num())
                , value_()
                , num_(1) {
                regenerate(7);
            }

            trx::trx(trx const &other)
                : gen_(cyng::crypto::make_rnd_num())
                , value_(other.value_)
                , num_(other.num_) {}

            void trx::regenerate(std::size_t length) {
                value_.resize(length);
                std::generate(value_.begin(), value_.end(), [&]() { return gen_.next(); });
            }

            trx &trx::operator++() {
                ++num_;
                return *this;
            }

            trx trx::operator++(int) {
                trx tmp(*this);
                ++num_;
                return tmp;
            }

            std::string trx::operator*() const { return value_ + "-" + std::to_string(num_); }

        } // namespace detail

        std::string generate_file_id() {

            static cyng::crypto::rnd gen("0123456789");

            std::string id;
            id.resize(12);
            std::generate(id.begin(), id.end(), [&]() { return gen.next(); });
            return id;
        }

        response_generator::response_generator()
            : trx_() {}

        cyng::tuple_t
        response_generator::public_open(std::string trx, cyng::buffer_t file_id, cyng::buffer_t client, std::string server) {

            return make_message(
                trx,
                1,
                0,
                msg_type::OPEN_RESPONSE,
                cyng::make_tuple(
                    cyng::null{}, // codepage
                    client,
                    file_id,
                    to_srv_id(server),
                    cyng::null{}, //  reference time
                    cyng::null{}  //  version
                    ),
                static_cast<std::uint16_t>(0xFFFF) //  crc placeholder
            );
        }

        cyng::tuple_t response_generator::public_close(std::string trx) {

            return make_message(
                trx,
                1,
                0,
                msg_type::CLOSE_RESPONSE,
                cyng::make_tuple(cyng::null{}),    // signature
                static_cast<std::uint16_t>(0xFFFF) //  crc placeholder
            );
        }

        cyng::tuple_t response_generator::get_proc_parameter(
            std::string const &trx,
            cyng::buffer_t const &server,
            cyng::obis_path_t const &path,
            cyng::tuple_t params) {

            return make_message(
                trx,
                1,
                0,
                msg_type::GET_PROC_PARAMETER_RESPONSE,
                cyng::make_tuple(server, path, params), // message: server id, path, parameters
                static_cast<std::uint16_t>(0xFFFF)      //  crc placeholder
            );
        }

        request_generator::request_generator(std::string const &name, std::string const &pwd)
            : trx_()
            , name_(name)
            , pwd_(pwd) {}

    } // namespace sml
} // namespace smf
