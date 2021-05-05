/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_VIF_H
#define SMF_MBUS_VIF_H

#include <cstdint>
#include <smf/mbus/units.h>

namespace smf
{
	namespace mbus	
	{
		enum class vib_category {

			CA,	//	Current [A]
			//CA01,	//	1111 1101 - 1101 nnnn - 1111 1100 - 0000 0001 
			//CA02,	//	1111 1101 - 1101 nnnn - 1111 1100 - 0000 0010
			//CA03,	//	1111 1101 - 1101 nnnn - 1111 1100 - 0000 0011 
			//CA04,	//	1111 1101 - 1101 nnnn - 1111 1100 - 0000 0100

			CL,	// Control
			//CL01,	//	1111 1101 - 0001 1111

			DP,	// Duration/Period
			//DP01,	//	0111 01nn
			//DP02,	//	0111 00nn
			//DP03,	//	1111 1101 - 0011 110n

			DT,	//	 Date / Time (Duration and Time stamp)
			//DT01,	//	0110 1101
			//DT02,	//	0110 1100
			//DT03,	//	1110 1101 - 0011 1100
			//DT04,	//	1110 1100 - 0011 1100

			EJ,	//	 Energy [GJ]
			//EJ01,
			//EJ02,
			//EJ03,
			//EJ04,
			//EJ05,
			//EJ06,

			EW,	//	 Energy [kWh]
			//EW01,	//	000 0nnn
			//EW02,	//	1111 1011 - 0000 000n
			//EW03,	//	1111 1011 - 1000 000n - 0111 1101
			//EW04,	//	1000 0nnn - 0011 1100
			//EW05,	//	1111 1011 - 1000 000n - 0011 1100
			//EW06,	//	1111 1011 - 1000 000n - 1111 1101 - 0011 1100 
			//EW07,	//	1000 0nnn - 1111 1100 - 0001 0000 
			//EW08,	//	1111 1011 - 1000 000n - 1111 1100 - 0001 0000
			//EW09,	//	1111 1011 - 1000 000n - 1111 1101 - 1111 1100 - 0001 0000

			FR,	//	 Frequency [Hz]
			//FR01,	//	1111 1011 - 0010 11nn

			HC,	//	 Heat Cost Allocation unit
			//HC01,	

			ID,	//	 Identification Numbers
			//ID01,	//	0111 100
			//ID02,	//	0111 1001 
			//ID03,	//	0111 1010
			//ID04,	//	1111 1101 - 0001 0001 
			//ID05,	//	1111 1101 - 0001 0000 

			MM,	//	Meter Management
			PD,	//	Phase in Degree
			PJ,	//	Power[kJ / h]
			PR,	//	Pressure[bar]
			PW,	//	Power[W]
			RE,	//	Reactive Energy[kvarh]
			RH,	//	Relative Humidity[%]
			RP,	//	Reactive Power[kvar]
			TC,	//	Temperature[°C]
			VF,	//	Volume Flow[m³ / h]
			VM,	//	Volume[m³]
			VV,	//	Voltage[V]
			YD,	//	Descriptor
		};

		/**
		 * value information field
		 * OMS-Spec_Vol2_AnnexB_C042.pdf
		 */
		class vif
		{
		public:
			explicit vif(char);
			explicit vif(std::uint8_t);
			vif() = delete;

			/**
			 * bit [0..6] Unit and multiplier (value)
			 */
			std::uint8_t get_value() const;

			/** @brief A VIB-Type describes a physical unit with a scaler and an optional VIF property like direction of flow.
			 * 
			 * @return scaler and unit
			 */
			std::pair<std::int8_t, unit> get_vib_type() const;

			/**
			 * bit [7]
			 */
			bool is_extended() const;

			/**
			 *  E111 1001 (Enhanced Identification)
			 *	Data = Identification No. (8 digit BCD)
			 */
			bool has_enhanced_id() const;

			/**
			 *  E111 1011 Extension of VIF-codes (true VIF is given in the first VIFE)
			 */
			bool is_ext_7B() const;

			/**
			 *  E111 1101 Extension of VIF-codes (true VIF is given in the first VIFE)
			 *	Multiplicative correction factor for value (not unit)
			 */
			bool is_ext_7D() const;

			/**
			 * E111 1111 (Manufacturer Specific)
			 */
			bool is_man_specific() const;

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
			std::uint8_t value_;
		};

		/**
		 * OMS-Spec_Vol2_AnnexB_D438.pdf
		 */
		class vib {

		};
		/**
		 * value information field
		 */
		//struct vif
		//{
		//	std::uint8_t val_ : 7;	//	[0..6] Unit and multiplier (value)
		//	std::uint8_t ext_ : 1;	//	[7] extension bit - Specifies if a VIFE Byte follows
		//};

		/**
		 * map union over dif bit field
		 */
		//union uvif
		//{
		//	std::uint8_t raw_;
		//	vif vif_;
		//};

		/**
		 * Up to 10 bytes
		 */
		//struct vife
		//{
		//	std::uint8_t val_ : 7;	//	[0..6] Contains Information on the single Value, such as Unit, Multiplicator, etc.
		//	std::uint8_t ext_ : 1;	//	[7] extension bit - Specifies if a VIFE Byte follows
		//};

		/**
		 *  Definition of the G format (coding the date)
		 */
		//struct format_G
		//{
		//	std::uint16_t day_ : 5;	//	1..31
		//	std::uint16_t y_ : 3;	//	
		//	std::uint16_t month_ : 4;	//	0..12
		//	std::uint16_t year_ : 4;	//	0..99

		//};

		//union uformat_G
		//{
		//	std::uint8_t raw_[2];
		//	format_G g_;
		//};

		/**
		 *  Definition of the F format (coding the date and time)
		 */
		//struct format_F
		//{
		//	std::uint16_t minute_ : 6;	//	0..59
		//	std::uint16_t reserved_2 : 2;
		//	std::uint16_t hour_ : 5;	//	0..23
		//	std::uint16_t reserved_1 : 3;
		//	format_G g_;
		//};

		//union uformat_F
		//{
		//	std::uint8_t raw_[4];
		//	format_F f_;
		//};

	}
}


#endif
