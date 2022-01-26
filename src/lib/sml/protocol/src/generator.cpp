#include <smf/mbus/server_id.h>
#include <smf/obis/defs.h>
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
                reset(7);
            }

            trx::trx(trx const &other)
                : gen_(cyng::crypto::make_rnd_num())
                , value_(other.value_)
                , num_(other.num_) {}

            void trx::reset(std::size_t length) {
                value_.resize(length);
                std::generate(value_.begin(), value_.end(), [&]() { return gen_.next(); });
                num_ = 1;
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

        cyng::tuple_t response_generator::get_list(cyng::buffer_t const &server, cyng::tuple_t val_list) {

            return make_message(
                *trx_,
                1, //  group
                0, //  abort code
                msg_type::GET_LIST_RESPONSE,
                cyng::make_tuple(
                    cyng::null{},                  // client id
                    server,                        // server id
                    cyng::null{},                  // list name
                    cyng::null{},                  // act. sensor time
                    val_list,                      // SML_List / SML_ListEntry
                    cyng::null{},                  //    listSignature
                    cyng::null{}),                 // actGatewayTime
                static_cast<std::uint16_t>(0xFFFF) //  crc placeholder
            );
        }

        request_generator::request_generator(std::string const &name, std::string const &pwd)
            : trx_()
            , group_no_(0)
            , name_(name)
            , pwd_(pwd) {}

        request_generator::request_generator(request_generator const &rg)
            : trx_(rg.trx_)
            , group_no_(rg.group_no_)
            , name_(rg.name_)
            , pwd_(rg.pwd_) {}

        void request_generator::reset(std::string const &name, std::string const &pwd, std::size_t length) {
            trx_.reset(length);
            group_no_ = 0;
            name_ = name;
            pwd_ = pwd;
        }

        cyng::tuple_t request_generator::public_open(cyng::mac48 client_id, cyng::buffer_t const &server_id) {

            return make_message(
                *trx_,
                group_no_++, //  group
                0,           //  abort code
                msg_type::OPEN_REQUEST,
                cyng::make_tuple(
                    cyng::null{},                  // code page
                    client_id,                     // client id
                    generate_file_id(),            // req file id
                    server_id,                     // server id
                    get_name(),                    // prevent moving
                    get_pwd(),                     // prevent moving
                    cyng::null{}),                 // sml-Version
                static_cast<std::uint16_t>(0xFFFF) // crc placeholder
            );
        }

        cyng::tuple_t request_generator::public_close() {
            group_no_ = 0;
            return make_message(
                *++trx_,
                group_no_, //  group
                0,         //  abort code
                msg_type::CLOSE_REQUEST,
                cyng::make_tuple(cyng::null{}),    // signature
                static_cast<std::uint16_t>(0xFFFF) // crc placeholder
            );
        }

        cyng::tuple_t request_generator::set_proc_parameter_reboot(cyng::buffer_t const &server_id) {
            return make_message(
                *++trx_,
                group_no_++,                          //  group
                0,                                    //  abort code
                msg_type::SET_PROC_PARAMETER_REQUEST, //  0x600
                cyng::make_tuple(
                    server_id,
                    get_name(),
                    get_pwd(),
                    cyng::make_tuple(OBIS_REBOOT), //  path entry
                    tree_empty(OBIS_REBOOT)),      //  params
                static_cast<std::uint16_t>(0xFFFF) // crc placeholder
            );
        }

        cyng::tuple_t request_generator::get_proc_parameter(cyng::buffer_t const &server_id, cyng::obis code) {
            BOOST_ASSERT(!name_.empty());
            return make_message(
                *++trx_,
                group_no_++,                          //  group
                0,                                    //  abort code
                msg_type::GET_PROC_PARAMETER_REQUEST, //  0x600
                // get process parameter request has 5 elements:
                //
                //	* server ID
                //	* username
                //	* password
                //	* parameter tree (OBIS)
                //	* attribute (not set = 01)
                cyng::make_tuple(
                    server_id,
                    get_name(),
                    get_pwd(),
                    cyng::make_tuple(code),        //   parameter tree (OBIS)
                    cyng::null{}),                 //  attribute
                static_cast<std::uint16_t>(0xFFFF) // crc placeholder
            );
        }

        cyng::tuple_t request_generator::get_proc_parameter(cyng::buffer_t const &server_id, cyng::obis_path_t path) {
            return make_message(
                *++trx_,
                group_no_++,                          //  group
                0,                                    //  abort code
                msg_type::GET_PROC_PARAMETER_REQUEST, //  0x600
                // get process parameter request has 5 elements:
                //
                //	* server ID
                //	* username
                //	* password
                //	* parameter tree (OBIS)
                //	* attribute (not set = 01)
                cyng::make_tuple(
                    server_id,
                    get_name(),
                    get_pwd(),
                    cyng::make_tuple(path),        //   parameter tree (OBIS)
                    cyng::null{}),                 //  attribute
                static_cast<std::uint16_t>(0xFFFF) // crc placeholder
            );
        }

        std::string const &request_generator::get_name() const { return name_; }
        std::string const &request_generator::get_pwd() const { return pwd_; }

    } // namespace sml
} // namespace smf
