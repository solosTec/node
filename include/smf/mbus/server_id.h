/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_SERVER_ID_H
#define SMF_MBUS_SERVER_ID_H

#include <cyng/obj/intrinsics/buffer.h>

#include <array>
#include <cstdint>

#include <boost/uuid/uuid.hpp>

namespace smf {
    /**
     * define an array that contains the server ID
     *
     * [0] 1 byte 01/02 01 == wireless, 02 == wired
     * [1-2] 2 bytes manufacturer ID
     * [3-6] 4 bytes serial number (reversed)
     * [7] 1 byte device type / media
     * [8] 1 byte product revision
     */
    using srv_id_t = std::array<std::uint8_t, 9>;

    /*
     * Example: 0x3105c => 96072000
     */
    cyng::buffer_t to_meter_id(std::uint32_t id);

    /*
     * Example: "10320047" => [10, 32, 00, 47]dec == [0A, 20, 00, 2F]hex
     */
    cyng::buffer_t to_meter_id(std::string id);

    /**
     * Produce a string with the format:
     * tt-mmmm-nnnnnnnn-vv-uu
     */
    std::string srv_id_to_str(srv_id_t);
    std::string srv_id_to_str(cyng::buffer_t);

    /**
     * Expects a string in the format:
     * tt-mmmm-nnnnnnnn-vv-uu
     */
    cyng::buffer_t to_srv_id(std::string);

    /**
     * @return meter id as string
     */
    std::string get_id(srv_id_t);
    cyng::buffer_t get_id_as_buffer(srv_id_t);

    /**
     * @return meter id as integer
     */
    std::uint32_t get_dev_id(srv_id_t);

    /**
     * @return 2 byte code
     *
     * Example:
     * @code
     * auto const manufacturer = mbus::decode(get_manufacturer_code(srv_id));
     * @endcode
     */
    std::pair<char, char> get_manufacturer_code(srv_id_t);

    constexpr std::uint8_t get_medium(srv_id_t address) noexcept { return address.at(8); }
    const char *get_medium_name(srv_id_t address) noexcept;

    /**
     * generates a metering code
     *
     * @param country_code 2 digit country code
     * @param server_id server ID
     */
    std::string gen_metering_code(std::string const &country_code, boost::uuids::uuid tag);

    /**
     * Fill a server id with buffer data
     */
    srv_id_t to_srv_id(cyng::buffer_t const &);

    enum class srv_type : std::uint32_t {
        MBUS_WIRED, //	M-Bus (long)
        MBUS_RADIO, //	M-Bus (long)
        W_MBUS,     //	wireless M-Bus (short)
        SERIAL,
        GW,
        BCD,    //	Rhein-Energie
        EON,    //	e-on
        DKE_1,  //	E DIN 43863-5:2010-02
        IMEI,   //	IMEI
        RWE,    //	RWE
        DKE_2,  //	E DIN 43863-5:2010-07
        SWITCH, //	outdated
        OTHER
    };
    /**
     * heuristical approach
     */
    srv_type detect_server_type(cyng::buffer_t const &);

} // namespace smf

#endif
