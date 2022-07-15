#include <smf/sml/generator.h>

#include <smf/mbus/server_id.h>
#include <smf/obis/defs.h>
#include <smf/sml/msg.h>
#include <smf/sml/value.hpp>

#include <cyng/obj/util.hpp>
#include <cyng/parse/string.h>

#include <algorithm>

#include <boost/assert.hpp>

namespace smf {
    namespace sml {

        trx::trx()
            : trx(7u) {}

        trx::trx(trx const &other)
            : gen_(cyng::crypto::make_rnd_num())
            , prefix_(other.prefix_)
            , num_(other.num_) {}

        trx::trx(std::size_t size)
            : gen_(cyng::crypto::make_rnd_num())
            , prefix_()
            , num_(1) {
            reset(size);
        }

        void trx::reset(std::size_t length) {
            prefix_.resize(length);
            reset();
        }

        void trx::reset() {
            std::generate(prefix_.begin(), prefix_.end(), [&]() { return gen_.next(); });
            num_ = 1;
            open_trx_.clear();
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

        std::string trx::operator*() const { return prefix_ + "-" + std::to_string(num_); }

        std::string const &trx::get_prefix() const { return prefix_; }

        std::size_t trx::store_pefix() {
            open_trx_.insert(get_prefix());
            return open_trx_.size();
        }

        std::pair<std::size_t, bool> trx::ack_trx(std::string const &trx) {
            //
            //  strip number
            //
            auto const parts = cyng::split(trx, "-");
            if (parts.size() == 2) {

                //
                //  search and remove
                //
                auto const b = open_trx_.erase(parts.at(0)) != 0;
                return {open_trx_.size(), b};
            }
            return {open_trx_.size(), false};
        }

        std::string generate_file_id() {

            static cyng::crypto::rnd gen("0123456789");

            std::string id;
            id.resize(12);
            std::generate(id.begin(), id.end(), [&]() { return gen.next(); });
            return id;
        }

        response_generator::response_generator() {}

        cyng::tuple_t
        response_generator::public_open(std::string trx, cyng::buffer_t file_id, cyng::buffer_t client, std::string server) {

            return make_message(
                trx,
                1,
                0,
                msg_type::OPEN_RESPONSE,
                cyng::make_tuple(
                    cyng::null{}, // codepage
                    to_srv_id(server),
                    trx,
                    client,
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

            BOOST_ASSERT_MSG(params.size() == 3 || params.empty(), "invalid parameter list");

            return make_message(
                trx,
                1,
                0,
                msg_type::GET_PROC_PARAMETER_RESPONSE,
                cyng::make_tuple(server, path, params), // message: server id, path, parameters
                static_cast<std::uint16_t>(0xFFFF)      //  crc placeholder
            );
        }

        cyng::tuple_t response_generator::get_list(
            std::string const &trx,
            cyng::buffer_t const &client,
            cyng::buffer_t const &server,
            cyng::obis code,
            cyng::tuple_t values,
            std::uint32_t seconds_idx) {

            // { null
            //. 01a815743145040102:binary
            //. null
            //. null
            //. { { 0100010800ff:binary
            //. . . 000202a0u32
            //. . . null
            //. . . 1eu8
            //. . . -1i8
            //. . . 14521i64
            //. . . null}
            //. . { 0100020800ff:binary
            //. . . 000202a0u32
            //. . . null
            //. . . 1eu8
            //. . . -1i8
            //. . . 940026i64
            //. . . null}
            //. . { 0100010801ff:binary
            //. . . null
            //. . . null
            //. . . 1eu8
            //. . . -1i8
            //. . . 0i64
            //. . . null}
            //. . { 0100020801ff:binary
            //. . . null
            //. . . null
            //. . . 1eu8
            //. . . -1i8
            //. . . 0i64
            //. . . null}
            //. . { 0100010802ff:binary
            //. . . null
            //. . . null
            //. . . 1eu8
            //. . . -1i8
            //. . . 14521i64
            //. . . null}
            //. . { 0100020802ff:binary
            //. . . null
            //. . . null
            //. . . 1eu8
            //. . . -1i8
            //. . . 940026i64
            //. . . null}
            //. . { 0100100700ff:binary
            //. . . null
            //. . . null
            //. . . 1bu8
            //. . . -1i8
            //. . . -138i32
            //. . . null}}
            //. null
            //. { 1u8
            //. . 0c1d636bu32}}

            if (cyng::is_nil(code)) {

                return make_message(
                    trx,
                    1, //  group
                    0, //  abort code
                    msg_type::GET_LIST_RESPONSE,
                    cyng::make_tuple(
                        client,                        // client id (optional)
                        server,                        // server id
                        cyng::null{},                  // list name (optional)
                        cyng::null{},                  // act. sensor time (optional)
                        values,                        // SML_List / SML_ListEntry
                        cyng::null{},                  // listSignature (optional)
                        make_attribute(seconds_idx)),  // actGatewayTime (optional)
                    static_cast<std::uint16_t>(0xFFFF) //  crc placeholder
                );
            }

            return make_message(
                trx,
                1, //  group
                0, //  abort code
                msg_type::GET_LIST_RESPONSE,
                cyng::make_tuple(
                    client,                        // client id (optional)
                    server,                        // server id
                    code,                          // list name (optional)
                    cyng::null{},                  // act. sensor time (optional)
                    values,                        // SML_List / SML_ListEntry
                    cyng::null{},                  // listSignature (optional)
                    make_attribute(seconds_idx)),  // actGatewayTime (optional)
                static_cast<std::uint16_t>(0xFFFF) //  crc placeholder
            );
        }

        cyng::tuple_t response_generator::get_list(cyng::buffer_t const &server, cyng::tuple_t data) {

            trx trx_mgr;
            return make_message(
                *trx_mgr,
                1, //  group
                0, //  abort code
                msg_type::GET_LIST_RESPONSE,
                cyng::make_tuple(
                    cyng::null{},                  // client id (optional)
                    server,                        // server id
                    cyng::null{},                  // list name (optional)
                    cyng::null{},                  // act. sensor time (optional)
                    data,                          // SML_List / SML_ListEntry
                    cyng::null{},                  // listSignature (optional)
                    make_attribute(0)),            // actGatewayTime (optional)
                static_cast<std::uint16_t>(0xFFFF) //  crc placeholder
            );
        }

        cyng::tuple_t response_generator::get_profile_list(
            std::string const &trx,
            cyng::buffer_t const &server,
            std::chrono::system_clock::time_point act_time, //  ToDo: use seconds index
            std::uint32_t reg_period,
            cyng::obis code,
            std::uint32_t val_time,
            std::uint64_t status,
            cyng::tuple_t &&data) {

            return make_message(
                trx,
                1,
                0,
                msg_type::GET_PROFILE_LIST_RESPONSE,
                cyng::make_tuple(
                    server,
                    act_time,
                    (reg_period == 0 ? 900u : reg_period),
                    cyng::obis_path_t({code}),
                    make_sec_index(val_time),
                    status,
                    std::move(data), // SML_List / SML_ListEntry
                    cyng::null{},    // rawdata
                    cyng::null{}     // signature
                    ),
                static_cast<std::uint16_t>(0xFFFF) // crc placeholder
            );
        }

        cyng::tuple_t response_generator::get_attention(
            std::string const &trx,
            cyng::buffer_t const &server,
            attention_type type,
            std::string txt) {

            return make_message(
                trx,
                1,
                0,
                msg_type::ATTENTION_RESPONSE,
                cyng::make_tuple(server, get_code(type), txt, cyng::null{}), // message: server id, code, type, message, details
                static_cast<std::uint16_t>(0xFFFF)                           // crc placeholder
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

            trx_.reset();
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
                    make_empty_tree(OBIS_REBOOT)), //  params
                static_cast<std::uint16_t>(0xFFFF) // crc placeholder
            );
        }

        cyng::tuple_t request_generator::get_proc_parameter(cyng::buffer_t const &server_id, cyng::obis code) {
            return get_proc_parameter(server_id, cyng::obis_path_t{code});
        }

        cyng::tuple_t request_generator::get_proc_parameter(cyng::buffer_t const &server_id, cyng::obis_path_t path) {
            BOOST_ASSERT(!name_.empty());
            return make_message(
                *++trx_,
                group_no_++,                          //  group
                0,                                    //  abort code
                msg_type::GET_PROC_PARAMETER_REQUEST, //  0x500
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
                    path,                          //   parameter tree (OBIS)
                    cyng::null{}),                 //  attribute
                static_cast<std::uint16_t>(0xFFFF) // crc placeholder
            );
        }

        cyng::tuple_t
        request_generator::set_proc_parameter(cyng::buffer_t const &server_id, cyng::obis_path_t path, cyng::attr_t attr) {
            BOOST_ASSERT(!path.empty());
            return make_message(
                *++trx_,
                group_no_++,                          //  group
                0,                                    //  abort code
                msg_type::SET_PROC_PARAMETER_REQUEST, //  0x600
                // get process parameter request has 5 elements:
                //
                //	* server ID
                //	* username
                //	* password
                //	* parameter tree path (OBIS)
                //	* parameter tree
                cyng::make_tuple(
                    server_id,
                    get_name(),
                    get_pwd(),
                    path,                                //  parameter tree path (OBIS)
                    make_param_tree(path.back(), attr)), //  parameter tree
                static_cast<std::uint16_t>(0xFFFF)       //  crc placeholder
            );
        }

        cyng::tuple_t request_generator::get_proc_parameter_access(
            cyng::buffer_t const &server_id,
            std::uint8_t role,
            std::uint8_t user,
            std::uint16_t device_index) {
            //
            //  build obis path
            //  * OBIS_ROOT_ACCESS_RIGHTS
            //  * 81818160[NN]FF - role
            //  * 81818160[NN][MM] - user
            //  * 81818164[NNMM] - device index (see response)
            //

            return get_proc_parameter(
                server_id,
                {OBIS_ROOT_ACCESS_RIGHTS, //  81 81 81 60 FF FF
                 cyng::make_obis(0x81, 0x81, 0x81, 0x60, role, 0xff),
                 cyng::make_obis(0x81, 0x81, 0x81, 0x60, role, user),
                 cyng::make_obis(0x81, 0x81, 0x81, 0x64, device_index)});
            // cyng::make_obis(0x81, 0x81, 0x81, 0x64, 1, 1)});
        }

        cyng::tuple_t request_generator::get_profile_list(
            cyng::buffer_t const &server_id,
            cyng::obis code,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point end) {
            return make_message(
                *++trx_,
                group_no_++,                        //  group
                0,                                  //  abort code
                msg_type::GET_PROFILE_LIST_REQUEST, //  0x400
                // get profile list request has 9 elements:
                //
                //	* server ID
                //	* username
                //	* password
                //  * withRawData (boolean)
                //  * begin time
                //  * end time
                //	* parameter tree (OBIS)
                //  * object_List (optional)
                //	* dasDetails
                //
                cyng::make_tuple(
                    server_id,
                    get_name(),
                    get_pwd(),
                    cyng::null{},                  // withRawData (boolean) - false
                    start,                         // start time (2/timestamp)
                    end,                           // end time (2/timestamp)
                    cyng::obis_path_t{code},       // parameter tree
                    cyng::null{},                  //  object_List
                    cyng::null{}),                 //  dasDetails
                static_cast<std::uint16_t>(0xFFFF) // crc placeholder
            );
        }

        std::string const &request_generator::get_name() const { return name_; }
        std::string const &request_generator::get_pwd() const { return pwd_; }
        trx &request_generator::get_trx_mgr() { return trx_; }

    } // namespace sml
} // namespace smf
