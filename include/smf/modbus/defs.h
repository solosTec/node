/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MODBUS_DEFS_H
#define SMF_MODBUS_DEFS_H

#include <cstdint>

namespace smf {
    namespace modbus {
        /**
         * function codes
         */
        enum fc : std::uint8_t {
            READ_COILS = 0x01,
            READ_DISCRETE_INPUTS = 0x02,
            READ_HOLDING_REGISTERS = 0x03,
            READ_INPUT_REGISTERS = 0x04,
            WRITE_SINGLE_COIL = 0x05,
            WRITE_SINGLE_REGISTER = 0x06,
            READ_EXCEPTION_STATUS = 0x07,
            WRITE_MULTIPLE_COILS = 0x0F,
            WRITE_MULTIPLE_REGISTERS = 0x10,
            REPORT_SLAVE_ID = 0x11,
            MASK_WRITE_REGISTER = 0x16,
            WRITE_AND_READ_REGISTERS = 0x17
        };

        constexpr std::uint8_t BROADCAST_ADDRESS = 0x00;

        constexpr std::uint32_t MAX_READ_BITS = 2000;
        constexpr std::uint32_t MAX_WRITE_BITS = 1968;

        constexpr std::uint8_t MAX_WR_WRITE_REGISTERS = 121;
        constexpr std::uint8_t MAX_WR_READ_REGISTERS = 125;
        constexpr std::uint8_t MAX_WRITE_REGISTERS = 123;
        constexpr std::uint8_t MAX_READ_REGISTERS = 125;

        constexpr std::uint8_t MAX_PDU_LENGTH = 253;
        constexpr std::uint32_t MAX_ADU_LENGTH = 260;

        /**
         * exceptions
         */
        enum class exc : std::uint8_t {
            ILLEGAL_FUNCTION = 0x01,
            ILLEGAL_DATA_ADDRESS,
            ILLEGAL_DATA_VALUE,
            SLAVE_OR_SERVER_FAILURE,
            ACKNOWLEDGE,
            SLAVE_OR_SERVER_BUSY,
            NEGATIVE_ACKNOWLEDGE,
            MEMORY_PARITY,
            NOT_DEFINED,
            GATEWAY_PATH,
            GATEWAY_TARGET,
            LAST
        };

    } // namespace modbus
} // namespace smf

#endif
