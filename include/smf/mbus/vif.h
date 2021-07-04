/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_VIF_H
#define SMF_MBUS_VIF_H

#include <smf/mbus/units.h>

#include <cyng/obj/intrinsics/obis.h>

#include <cstdint>

namespace smf {
    namespace mbus {
        enum class vib_category {

            CA,   //	Current [A]
            CA01, //	1111 1101 - 1101 nnnn - 1111 1100 - 0000 0001
            CA02, //	1111 1101 - 1101 nnnn - 1111 1100 - 0000 0010
            CA03, //	1111 1101 - 1101 nnnn - 1111 1100 - 0000 0011
            CA04, //	1111 1101 - 1101 nnnn - 1111 1100 - 0000 0100

            CL,   // Control
            CL01, //	1111 1101 - 0001 1111

            DP,   // Duration/Period
            DP01, //	0111 01nn
            DP02, //	0111 00nn
            DP03, //	1111 1101 - 0011 110n

            DT,   //	 Date / Time (Duration and Time stamp)
            DT01, //	0110 1101
            DT02, //	0110 1100
            DT03, //	1110 1101 - 0011 1100
            DT04, //	1110 1100 - 0011 1100

            EJ,   //	 Energy [GJ]
            EJ01, //  0000 1nn
            EJ02,
            EJ03,
            EJ04,
            EJ05,
            EJ06,

            EW,   //	 Energy [kWh]
            EW01, //	000 0nnn
            EW02, //	1111 1011 - 0000 000n
            EW03, //	1111 1011 - 1000 000n - 0111 1101
            EW04, //	1000 0nnn - 0011 1100
            EW05, //	1111 1011 - 1000 000n - 0011 1100
            EW06, //	1111 1011 - 1000 000n - 1111 1101 - 0011 1100
            EW07, //	1000 0nnn - 1111 1100 - 0001 0000
            EW08, //	1111 1011 - 1000 000n - 1111 1100 - 0001 0000
            EW09, //	1111 1011 - 1000 000n - 1111 1101 - 1111 1100 - 0001 0000

            FR,   //	 Frequency [Hz]
            FR01, //	1111 1011 - 0010 11nn

            HC,   //	 Heat Cost Allocation unit
            HC01, //  0110 1110
            HC02, //  1110 1110 1111 1100 0001 0001

            ID,   //	 Identification Numbers
            ID01, //	0111 100
            ID02, //	0111 1001
            ID03, //	0111 1010
            ID04, //	1111 1101 - 0001 0001
            ID05, //	1111 1101 - 0001 0000
            ID06, //	1111 1101 - 0001 0000

            MM,   //    Meter Management
            MM09, //    1111 1101 0111 0100 days Remaining battery life time

            PD,   //	Phase in Degree
            PJ,   //	Power[kJ / h]
            PJ01, //	0011 0nnn

            PR, //	Pressure[bar]

            PW,   //	Power[W]
            PW01, //  0010 1nnn
            PW03, //  1010 1nnn 0011 1100
            PW04, //  1111 1011 0111 1nnn
            PW06, //  1111 1011 1111 1nnn 0011 1100
            PW07, //  1010 1nnn 1111 1100 0001 0000
            PW08, //  1111 1011 1010 100n 1111 1100 0001 0000
            PW09, //  1010 1nnn 1111 1100 0000 1100
            PW10, //  1010 1nnn 1111 1100 0000 1100

            RE, //	Reactive Energy[kvarh]
            RH, //	Relative Humidity[%]
            RP, //	Reactive Power[kvar]

            TC,   //	Temperature[°C]
            TC01, //  0101 10nn
            TC02, //  0101 11nn
            TC03, //  1101 10nn 0011 1110

            VF,   //	Volume Flow[m³ / h]
            VF01, // 0011 1nnn
            VF02, // 1011 1nnn 0011 1010
            VF03, // 1011 1nnn 0011 1110

            VM, //	Volume[m³]
            VM01,
            VM02,
            //  VM10, ...

            VV, //	Voltage[V]
            YD, //	Descriptor
            UNDEF,
        };

        std::string to_string(vib_category);

        //
        //  forward declaration(s)
        //
        class dif;
        class dife;
        class vif;
        class vife;

        /**
         *  get the VIB category
         */
        std::tuple<vib_category, cyng::obis, std::int8_t, unit, bool>
        get_vib_category(std::uint8_t medium, std::uint8_t tariff, vif const &);

        /**
         * value information field
         * OMS-Spec_Vol2_AnnexB_C042.pdf
         */
        class vif {
            friend std::tuple<vib_category, cyng::obis, std::int8_t, unit, bool>
            get_vib_category(std::uint8_t medium, std::uint8_t tariff, vif const &);

          public:
            explicit vif(char);
            explicit vif(std::uint8_t);
            vif() = delete;

            /**
             * get raw (value)
             */
            std::uint8_t get_value() const;

            /**
             * get extended value (VIFE)
             */
            std::uint8_t get_ext_value(std::size_t) const;

            /**
             * value & N
             */
            std::uint8_t get_and(std::uint8_t) const;

            /** @brief A VIB-Type describes a physical unit with a scaler and an optional VIF property like direction of flow.
             *
             * @return scaler and unit
             */
            std::tuple<std::int8_t, unit, cyng::obis> get_vib_type(std::uint8_t medium, dif const &) const;

            /**
             * bit [7]
             */
            bool is_extended() const;

            /**
             * bit [7]
             * Check the entry at the back.
             */
            bool is_complete() const;

            /**
             * add extension code
             */
            std::size_t add(char);

            /**
             * @return 0 if no vife is available
             */
            std::size_t get_vife_size() const;

            /**
             * @return true if element with the specified index has
             * value 0xFF
             */
            bool is_manufacturer_specific(std::size_t) const;

            /**
             * DT02, DT04
             *
             * @return true if date flag is set
             */
            bool is_date() const;

            /**
             * DT01, DT03
             *
             * @return true if time flag is set
             */
            bool is_time() const;

          private:
            std::uint8_t get_last() const;

          private:
            std::vector<std::uint8_t> value_;
        };

        /**
         *  Definition of the G format (coding the date)
         */
        // struct format_G
        //{
        //	std::uint16_t day_ : 5;	//	1..31
        //	std::uint16_t y_ : 3;	//
        //	std::uint16_t month_ : 4;	//	0..12
        //	std::uint16_t year_ : 4;	//	0..99

        //};

        // union uformat_G
        //{
        //	std::uint8_t raw_[2];
        //	format_G g_;
        //};

        /**
         *  Definition of the F format (coding the date and time)
         */
        // struct format_F
        //{
        //	std::uint16_t minute_ : 6;	//	0..59
        //	std::uint16_t reserved_2 : 2;
        //	std::uint16_t hour_ : 5;	//	0..23
        //	std::uint16_t reserved_1 : 3;
        //	format_G g_;
        //};

        // union uformat_F
        //{
        //	std::uint8_t raw_[4];
        //	format_F f_;
        //};

    } // namespace mbus
} // namespace smf

#endif
